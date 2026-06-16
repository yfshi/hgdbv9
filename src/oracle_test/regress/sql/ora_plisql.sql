--
-- PLISQL
--

-- return is a non-reserved keyword, Can be used as object name.
CREATE TABLE RETURN (fooid INT, foosubid INT, fooname TEXT);
INSERT INTO RETURN VALUES (1, 2, 'three');
INSERT INTO RETURN VALUES (4, 5, 'six');
SELECT * FROM RETURN;

-- Creating a parameterless function cannot have an empty parenthesis.
-- FAILED
CREATE or replace FUNCTION ora_func() RETURN integer AS
BEGIN    
    RETURN 1;
END;
/

-- OK
CREATE or replace FUNCTION ora_func RETURN integer AS
BEGIN    
    RETURN 1;
END;
/
select ora_func() from dual;

-- NOCOPY.
CREATE OR REPLACE FUNCTION test_nocopy(a IN int, b OUT NOCOPY int, c IN OUT NOCOPY int) 
RETURN int 
IS
BEGIN 
   b := a;
   c := a;
   return 1;
END;
/

-- The outermost DECLARE block of a function does not have the DECLARE keyword.
CREATE OR REPLACE
FUNCTION ora_func (num1 IN int, num2 IN int)
RETURN int
AS
 num3 int :=10;
 num4 int :=10;
 num5 int;
BEGIN
 num3 := num1 + num2;
 num4 := num1 * num2;
 num5 := num3 * num4;
RETURN num5;
END;
/

select ora_func(5,9)from dual;

-- IS keyword
CREATE or replace EDITIONABLE FUNCTION ora_func RETURN integer IS
BEGIN    
    RETURN 1;
END;
/

--
-- Only syntactically compatible and do not implement functionality. 
-- therefore the identifiers and objects etc used in these only syntactically 
-- compatible clauses are dummy.
--

-- EDITIONABLE
CREATE or replace EDITIONABLE FUNCTION ora_func RETURN integer IS
BEGIN    
    RETURN 1;
END;
/

-- NONEDITIONABLE
CREATE or replace NONEDITIONABLE FUNCTION ora_func RETURN integer IS
BEGIN    
    RETURN 1;
END;
/

-- sharing_clause
CREATE or replace FUNCTION ora_func RETURN integer
SHARING = METADATA
IS
BEGIN    
    RETURN 1;
END;
/

CREATE or replace FUNCTION ora_func RETURN integer
SHARING = NONE
IS
BEGIN    
    RETURN 1;
END;
/

-- invoker_rights_clause
CREATE or replace FUNCTION ora_func RETURN integer
SHARING = NONE AUTHID CURRENT_USER
IS
BEGIN    
    RETURN 1;
END;
/

CREATE or replace FUNCTION ora_func RETURN integer
SHARING = NONE AUTHID DEFINER
IS
BEGIN    
    RETURN 1;
END;
/

-- accessible_by_clause
CREATE or replace FUNCTION ora_func RETURN integer
SHARING = NONE AUTHID DEFINER ACCESSIBLE BY ( B )
IS
BEGIN    
    RETURN 1;
END;
/

CREATE or replace FUNCTION ora_func RETURN integer
SHARING = NONE AUTHID DEFINER ACCESSIBLE BY ( A.B )
IS
BEGIN    
    RETURN 1;
END;
/

CREATE or replace FUNCTION ora_func RETURN integer
SHARING = NONE AUTHID DEFINER ACCESSIBLE BY ( FUNCTION A.B )
IS
BEGIN    
    RETURN 1;
END;
/

CREATE or replace FUNCTION ora_func RETURN integer
SHARING = NONE AUTHID DEFINER
ACCESSIBLE BY ( FUNCTION A.B, PROCEDURE C.D )
IS
BEGIN    
    RETURN 1;
END;
/

CREATE or replace FUNCTION ora_func RETURN integer
SHARING = NONE AUTHID DEFINER
ACCESSIBLE BY ( FUNCTION A.B, PROCEDURE C.D, PACKAGE E,
TRIGGER F, TYPE G )
IS
BEGIN    
    RETURN 1;
END;
/

-- default_collation_clause
CREATE or replace FUNCTION ora_func RETURN integer
SHARING = NONE AUTHID DEFINER
ACCESSIBLE BY ( FUNCTION A.B, PROCEDURE C.D )
DEFAULT COLLATION USING_NLS_COMP
IS
BEGIN    
    RETURN 1;
END;
/

-- deterministic_clause
CREATE or replace FUNCTION ora_func RETURN integer
SHARING = NONE AUTHID DEFINER
ACCESSIBLE BY ( FUNCTION A.B, PROCEDURE C.D )
DEFAULT COLLATION USING_NLS_COMP
DETERMINISTIC
IS
BEGIN    
    RETURN 1;
END;
/

-- parallel_enable_clause
CREATE or replace FUNCTION ora_func RETURN integer
SHARING = NONE AUTHID DEFINER
ACCESSIBLE BY ( FUNCTION A.B, PROCEDURE C.D )
DEFAULT COLLATION USING_NLS_COMP
DETERMINISTIC
PARALLEL_ENABLE
IS
BEGIN    
    RETURN 1;
END;
/

CREATE or replace FUNCTION ora_func RETURN integer
SHARING = NONE AUTHID DEFINER
ACCESSIBLE BY ( FUNCTION A.B, PROCEDURE C.D )
DEFAULT COLLATION USING_NLS_COMP
DETERMINISTIC
PARALLEL_ENABLE ( PARTITION A BY ANY )
IS
BEGIN    
    RETURN 1;
END;
/

CREATE or replace FUNCTION ora_func RETURN integer
SHARING = NONE AUTHID DEFINER
ACCESSIBLE BY ( FUNCTION A.B, PROCEDURE C.D )
DEFAULT COLLATION USING_NLS_COMP
DETERMINISTIC
PARALLEL_ENABLE ( PARTITION A BY VALUE ( B ) )
IS
BEGIN    
    RETURN 1;
END;
/

CREATE or replace FUNCTION ora_func RETURN integer
SHARING = NONE AUTHID DEFINER
ACCESSIBLE BY ( FUNCTION A.B, PROCEDURE C.D )
DEFAULT COLLATION USING_NLS_COMP
DETERMINISTIC
PARALLEL_ENABLE ( PARTITION A BY HASH ( B, C ) )
IS
BEGIN    
    RETURN 1;
END;
/

CREATE or replace FUNCTION ora_func RETURN integer
SHARING = NONE AUTHID DEFINER
ACCESSIBLE BY ( FUNCTION A.B, PROCEDURE C.D )
DEFAULT COLLATION USING_NLS_COMP
DETERMINISTIC
PARALLEL_ENABLE ( PARTITION A BY HASH ( B, C ) ORDER A BY ( E,F ) )
IS
BEGIN    
    RETURN 1;
END;
/

CREATE or replace FUNCTION ora_func RETURN integer
SHARING = NONE AUTHID DEFINER
ACCESSIBLE BY ( FUNCTION A.B, PROCEDURE C.D )
DEFAULT COLLATION USING_NLS_COMP
DETERMINISTIC
PARALLEL_ENABLE ( PARTITION A BY HASH ( B, C ) CLUSTER A BY ( E,F ) )
IS
BEGIN    
    RETURN 1;
END;
/

CREATE or replace FUNCTION ora_func RETURN integer
SHARING = NONE AUTHID DEFINER
ACCESSIBLE BY ( FUNCTION A.B, PROCEDURE C.D )
DEFAULT COLLATION USING_NLS_COMP
DETERMINISTIC
PARALLEL_ENABLE ( PARTITION A BY RANGE ( B, C ) )
IS
BEGIN    
    RETURN 1;
END;
/

CREATE or replace FUNCTION ora_func RETURN integer
SHARING = NONE AUTHID DEFINER
ACCESSIBLE BY ( FUNCTION A.B, PROCEDURE C.D )
DEFAULT COLLATION USING_NLS_COMP
DETERMINISTIC
PARALLEL_ENABLE ( PARTITION A BY RANGE ( B, C ) ORDER A BY ( E,F ) )
IS
BEGIN    
    RETURN 1;
END;
/

CREATE or replace FUNCTION ora_func RETURN integer
SHARING = NONE AUTHID DEFINER
ACCESSIBLE BY ( FUNCTION A.B, PROCEDURE C.D )
DEFAULT COLLATION USING_NLS_COMP
DETERMINISTIC
PARALLEL_ENABLE ( PARTITION A BY RANGE ( B, C ) CLUSTER A BY ( E,F ) )
IS
BEGIN    
    RETURN 1;
END;
/

-- result_cache_clause
CREATE or replace FUNCTION ora_func RETURN integer
SHARING = NONE AUTHID DEFINER
ACCESSIBLE BY ( FUNCTION A.B, PROCEDURE C.D )
DEFAULT COLLATION USING_NLS_COMP
DETERMINISTIC
PARALLEL_ENABLE ( PARTITION A BY RANGE ( B, C ) CLUSTER A BY ( E,F ) )
RESULT_CACHE
IS
BEGIN    
    RETURN 1;
END;
/

CREATE or replace FUNCTION ora_func RETURN integer
SHARING = NONE AUTHID DEFINER
ACCESSIBLE BY ( FUNCTION A.B, PROCEDURE C.D )
DEFAULT COLLATION USING_NLS_COMP
DETERMINISTIC
PARALLEL_ENABLE ( PARTITION A BY RANGE ( B, C ) CLUSTER A BY ( E,F ) )
RESULT_CACHE RELIES_ON ()
IS
BEGIN    
    RETURN 1;
END;
/

CREATE or replace FUNCTION ora_func RETURN integer
SHARING = NONE AUTHID DEFINER
ACCESSIBLE BY ( FUNCTION A.B, PROCEDURE C.D )
DEFAULT COLLATION USING_NLS_COMP
DETERMINISTIC
PARALLEL_ENABLE ( PARTITION A BY RANGE ( B, C ) CLUSTER A BY ( E,F ) )
RESULT_CACHE RELIES_ON ( data_source1, data_source2)
IS
BEGIN    
    RETURN 1;
END;
/

-- aggregate_clause
CREATE or replace FUNCTION ora_func RETURN integer
SHARING = NONE AUTHID DEFINER
ACCESSIBLE BY ( FUNCTION A.B, PROCEDURE C.D )
DEFAULT COLLATION USING_NLS_COMP
DETERMINISTIC
PARALLEL_ENABLE ( PARTITION A BY RANGE ( B, C ) CLUSTER A BY ( E,F ) )
RESULT_CACHE RELIES_ON ( data_source1, data_source2)
AGGREGATE USING pg_catalog.int4
IS
BEGIN    
    RETURN 1;
END;
/

CREATE or replace FUNCTION ora_func RETURN integer
SHARING = NONE AUTHID DEFINER
ACCESSIBLE BY ( FUNCTION A.B, PROCEDURE C.D )
DEFAULT COLLATION USING_NLS_COMP
DETERMINISTIC
PARALLEL_ENABLE ( PARTITION A BY RANGE ( B, C ) CLUSTER A BY ( E,F ) )
RESULT_CACHE RELIES_ON ( data_source1, data_source2)
AGGREGATE USING int
IS
BEGIN    
    RETURN 1;
END;
/

-- pipelined_clause
CREATE or replace FUNCTION ora_func RETURN integer
SHARING = NONE AUTHID DEFINER
ACCESSIBLE BY ( FUNCTION A.B, PROCEDURE C.D )
DEFAULT COLLATION USING_NLS_COMP
DETERMINISTIC
PARALLEL_ENABLE ( PARTITION A BY RANGE ( B, C ) CLUSTER A BY ( E,F ) )
RESULT_CACHE RELIES_ON ( data_source1, data_source2)
AGGREGATE USING int
PIPELINED
IS
BEGIN    
    RETURN 1;
END;
/

CREATE or replace FUNCTION ora_func RETURN integer
SHARING = NONE AUTHID DEFINER
ACCESSIBLE BY ( FUNCTION A.B, PROCEDURE C.D )
DEFAULT COLLATION USING_NLS_COMP
DETERMINISTIC
PARALLEL_ENABLE ( PARTITION A BY RANGE ( B, C ) CLUSTER A BY ( E,F ) )
RESULT_CACHE RELIES_ON ( data_source1, data_source2)
AGGREGATE USING int
PIPELINED USING pg_catalog.int4
IS
BEGIN    
    RETURN 1;
END;
/

CREATE or replace FUNCTION ora_func RETURN integer
SHARING = NONE AUTHID DEFINER
ACCESSIBLE BY ( FUNCTION A.B, PROCEDURE C.D )
DEFAULT COLLATION USING_NLS_COMP
DETERMINISTIC
PARALLEL_ENABLE ( PARTITION A BY RANGE ( B, C ) CLUSTER A BY ( E,F ) )
RESULT_CACHE RELIES_ON ( data_source1, data_source2)
AGGREGATE USING int
PIPELINED ROW POLYMORPHIC
IS
BEGIN    
    RETURN 1;
END;
/

CREATE or replace FUNCTION ora_func RETURN integer
SHARING = NONE AUTHID DEFINER
ACCESSIBLE BY ( FUNCTION A.B, PROCEDURE C.D )
DEFAULT COLLATION USING_NLS_COMP
DETERMINISTIC
PARALLEL_ENABLE ( PARTITION A BY RANGE ( B, C ) CLUSTER A BY ( E,F ) )
RESULT_CACHE RELIES_ON ( data_source1, data_source2)
AGGREGATE USING int
PIPELINED ROW POLYMORPHIC USING pg_catalog.int4
IS
BEGIN    
    RETURN 1;
END;
/

CREATE or replace FUNCTION ora_func RETURN integer
SHARING = NONE AUTHID DEFINER
ACCESSIBLE BY ( FUNCTION A.B, PROCEDURE C.D )
DEFAULT COLLATION USING_NLS_COMP
DETERMINISTIC
PARALLEL_ENABLE ( PARTITION A BY RANGE ( B, C ) CLUSTER A BY ( E,F ) )
RESULT_CACHE RELIES_ON ( data_source1, data_source2)
AGGREGATE USING int
PIPELINED TABLE POLYMORPHIC USING pg_catalog.int4
IS
BEGIN    
    RETURN 1;
END;
/

-- sql_macro_clause
CREATE or replace FUNCTION ora_func RETURN integer
SHARING = NONE AUTHID DEFINER
ACCESSIBLE BY ( FUNCTION A.B, PROCEDURE C.D )
DEFAULT COLLATION USING_NLS_COMP
DETERMINISTIC
PARALLEL_ENABLE ( PARTITION A BY RANGE ( B, C ) CLUSTER A BY ( E,F ) )
RESULT_CACHE RELIES_ON ( data_source1, data_source2)
AGGREGATE USING int
PIPELINED TABLE POLYMORPHIC USING pg_catalog.int4
SQL_MACRO 
IS
BEGIN    
    RETURN 1;
END;
/

CREATE or replace FUNCTION ora_func RETURN integer
SHARING = NONE AUTHID DEFINER
ACCESSIBLE BY ( FUNCTION A.B, PROCEDURE C.D )
DEFAULT COLLATION USING_NLS_COMP
DETERMINISTIC
PARALLEL_ENABLE ( PARTITION A BY RANGE ( B, C ) CLUSTER A BY ( E,F ) )
RESULT_CACHE RELIES_ON ( data_source1, data_source2)
AGGREGATE USING int
PIPELINED TABLE POLYMORPHIC USING pg_catalog.int4
SQL_MACRO ( scalar )
IS
BEGIN    
    RETURN 1;
END;
/

CREATE or replace FUNCTION ora_func RETURN integer
SHARING = NONE AUTHID DEFINER
ACCESSIBLE BY ( FUNCTION A.B, PROCEDURE C.D )
DEFAULT COLLATION USING_NLS_COMP
DETERMINISTIC
PARALLEL_ENABLE ( PARTITION A BY RANGE ( B, C ) CLUSTER A BY ( E,F ) )
RESULT_CACHE RELIES_ON ( data_source1, data_source2)
AGGREGATE USING int
PIPELINED TABLE POLYMORPHIC USING pg_catalog.int4
SQL_MACRO ( type => scalar )
IS
BEGIN    
    RETURN 1;
END;
/

CREATE or replace FUNCTION ora_func RETURN integer
SHARING = NONE AUTHID DEFINER
ACCESSIBLE BY ( FUNCTION A.B, PROCEDURE C.D )
DEFAULT COLLATION USING_NLS_COMP
DETERMINISTIC
PARALLEL_ENABLE ( PARTITION A BY RANGE ( B, C ) CLUSTER A BY ( E,F ) )
RESULT_CACHE RELIES_ON ( data_source1, data_source2)
AGGREGATE USING int
PIPELINED TABLE POLYMORPHIC USING pg_catalog.int4
SQL_MACRO ( table )
IS
BEGIN    
    RETURN 1;
END;
/

CREATE or replace FUNCTION ora_func RETURN integer
SHARING = NONE AUTHID DEFINER
ACCESSIBLE BY ( FUNCTION A.B, PROCEDURE C.D )
DEFAULT COLLATION USING_NLS_COMP
DETERMINISTIC
PARALLEL_ENABLE ( PARTITION A BY RANGE ( B, C ) CLUSTER A BY ( E,F ) )
RESULT_CACHE RELIES_ON ( data_source1, data_source2)
AGGREGATE USING int
PIPELINED TABLE POLYMORPHIC USING pg_catalog.int4
SQL_MACRO ( type => table )
IS
BEGIN    
    RETURN 1;
END;
/

--
-- procedure
--

-- The outermost DECLARE block of a procedure does not have the DECLARE keyword.
CREATE OR REPLACE PROCEDURE ora_procedure()
AS
	p integer := 20;   
begin
	raise notice '%', p;
end;
/

call ora_procedure();

-- IS keyword
CREATE OR REPLACE PROCEDURE ora_procedure
IS
	p integer := 20;   
begin
	raise notice '%', p;
end;
/

-- EDITIONABLE
CREATE OR REPLACE EDITIONABLE PROCEDURE ora_procedure
IS
	p integer := 20;   
begin
	raise notice '%', p;
end;
/

-- NONEDITIONABLE
CREATE OR REPLACE NONEDITIONABLE PROCEDURE ora_procedure
IS
	p integer := 20;   
begin
	raise notice '%', p;
end;
/

-- sharing_clause 
CREATE OR REPLACE PROCEDURE ora_procedure
SHARING = METADATA
IS
	p integer := 20;   
begin
	raise notice '%', p;
end;
/

CREATE OR REPLACE PROCEDURE ora_procedure
SHARING = NONE
IS
	p integer := 20;   
begin
	raise notice '%', p;
end;
/

-- default_collation_clause
CREATE OR REPLACE PROCEDURE ora_procedure
SHARING = METADATA
DEFAULT COLLATION USING_NLS_COMP
IS
	p integer := 20;   
begin
	raise notice '%', p;
end;
/

-- invoker_rights_clause
CREATE OR REPLACE PROCEDURE ora_procedure
SHARING = METADATA
DEFAULT COLLATION USING_NLS_COMP
AUTHID CURRENT_USER
IS
	p integer := 20;   
begin
	raise notice '%', p;
end;
/

CREATE OR REPLACE PROCEDURE ora_procedure
SHARING = METADATA
DEFAULT COLLATION USING_NLS_COMP
AUTHID DEFINER
IS
	p integer := 20;   
begin
	raise notice '%', p;
end;
/

-- accessible_by_clause
CREATE OR REPLACE PROCEDURE ora_procedure
SHARING = METADATA
DEFAULT COLLATION USING_NLS_COMP
AUTHID CURRENT_USER
ACCESSIBLE BY ( B )
IS
	p integer := 20;   
begin
	raise notice '%', p;
end;
/

CREATE OR REPLACE PROCEDURE ora_procedure
SHARING = METADATA
DEFAULT COLLATION USING_NLS_COMP
AUTHID CURRENT_USER
ACCESSIBLE BY ( A.B )
IS
	p integer := 20;   
begin
	raise notice '%', p;
end;
/

CREATE OR REPLACE PROCEDURE ora_procedure
SHARING = METADATA
DEFAULT COLLATION USING_NLS_COMP
AUTHID CURRENT_USER
ACCESSIBLE BY ( FUNCTION A.B )
IS
	p integer := 20;   
begin
	raise notice '%', p;
end;
/

CREATE OR REPLACE PROCEDURE ora_procedure
SHARING = METADATA
DEFAULT COLLATION USING_NLS_COMP
AUTHID CURRENT_USER
ACCESSIBLE BY ( FUNCTION A.B, PROCEDURE C.D )
IS
	p integer := 20;   
begin
	raise notice '%', p;
end;
/

CREATE OR REPLACE PROCEDURE ora_procedure
SHARING = METADATA
DEFAULT COLLATION USING_NLS_COMP
AUTHID CURRENT_USER
ACCESSIBLE BY ( FUNCTION A.B, PROCEDURE C.D, PACKAGE E, TRIGGER F, TYPE G )
IS
	p integer := 20;   
begin
	raise notice '%', p;
end;
/

--
-- clean
--
DROP TABLE RETURN;
DROP FUNCTION ora_func();
DROP FUNCTION test_nocopy(a IN int, b OUT NOCOPY int, c IN OUT NOCOPY int);
DROP PROCEDURE ora_procedure();

--test alter function compile or editionable
create or replace function test_func(id integer) return integer is
var1 integer;
begin
  var1 := 23;
  return var1 + 2;
end;
/

create or replace function test_pg_func(id integer) returns integer as $$
begin
  return 23;
end;
$$ LANGUAGE PLPGSQL;
/

alter function test_func editionable;
alter function public.test_func noneditionable;
alter function test_func compile;
alter function test_func compile debug;
alter function test_func compile debug sd = mv;
alter function test_func compile debug reuse settings;

-----alter pg function
alter function test_pg_func editionable;
alter function public.test_pg_func noneditionable;
alter function public.test_pg_func compile;
alter function public.test_pg_func compile debug;
alter function public.test_pg_func compile debug sd = mv;
alter function public.test_pg_func compile debug reuse settings;

--drop function
drop function test_func(integer);
drop function test_pg_func(integer);

--test alter procedure compile or editionable
create or replace procedure test_proc(id integer) is
  var1 integer := 10;
begin
  RAISE NOTICE 'in test_proc, var1 = %', var1;
end;
/

create or replace procedure test_pg_proc(id integer) AS $$
declare
  var1 integer := 20;
begin
  raise notice 'in test_pg_proc, var1 = %', var1;
end;
$$ LANGUAGE plpgsql;
/

alter procedure test_proc editionable;
alter procedure public.test_proc noneditionable;
alter procedure test_proc compile;
alter procedure test_proc compile debug;
alter procedure test_proc compile debug sd = mv;
alter procedure test_proc compile debug reuse settings;

-----alter pg procedure
alter procedure test_pg_proc editionable;
alter procedure public.test_pg_proc noneditionable;
alter procedure public.test_pg_proc compile;
alter procedure public.test_pg_proc compile debug;
alter procedure public.test_pg_proc compile debug sd = mv;
alter procedure public.test_pg_proc compile debug reuse settings;

--drop procedure
drop function test_proc;--error
drop procedure test_proc;
drop procedure test_pg_proc(integer);

--
-- Oracle anonymous block
--
begin
null;
end;
/

declare
begin
null;
end;
/

<<main>>
begin
null;
end;
/


<<main>>
declare
begin
null;
end;
/


<<main>>
declare
i integer := 10;
begin
 raise notice '%', i;
 raise notice '%', main.i;
end;
/

<<main>>
i integer := 10;
begin
 raise notice '%', i;
 raise notice '%', main.i;
end;
/

<< main >>
begin
null;
end;
/

<<
main >>
begin
null;
end;
/

<<
main
>>
begin
null;
end;
/

declare
i integer := 10;
begin
 raise notice '%', i;
end;
/

DECLARE
  grade CHAR(1);
BEGIN
  grade := 'B';

  CASE grade
    WHEN 'A' THEN raise notice 'Excellent';
    WHEN 'B' THEN raise notice 'Very Good';
  END CASE;
EXCEPTION
  WHEN CASE_NOT_FOUND THEN
    raise notice 'No such grade';
END;
/

declare
"a b c end; " text;
a_output text;
begin
"a b c end; " := ' end;  ';
raise notice '%', "a b c end; ";

a_output := "a b c end; ";
raise notice '%', a_output;

a_output := '''end;''';
raise notice '%', a_output;

a_output := '''''end;''''';
raise notice '%', a_output;
a_output := 'end;
'' '''' ''''''
''
''''
''''''
"
end;   ';
raise notice '%', a_output;
end;
/

--
-- Compatible with Oracle:
-- Parentheses can be omitted when calling parameterless functions
--
create or replace function f_noparentheses
return int is
begin
return 11;
end;
/

create table t_noparentheses(f_noparentheses int);
insert into t_noparentheses values(123);
select f_noparentheses from t_noparentheses;

-- When the function name and the column name are the same, we preferentially resolve to the column name.
select f_noparentheses from dual;
select f_noparentheses from t_noparentheses;

/* Begin - BUG:Z139 */
CREATE OR REPLACE FUNCTION bug139_func RETURN integer
AUTHID DEFINER
DETERMINISTIC
PARALLEL_ENABLE ( PARTITION A BY RANGE ( B, C ) CLUSTER A BY ( E,F ) )
IS
BEGIN    
    RETURN 1;
END;
/

CREATE OR REPLACE PROCEDURE bug139_proc
AUTHID CURRENT_USER
IS
	p integer := 20;   
begin
	raise notice '%', p;
end;
/

-- nested subproc of plisql
create or replace function test_subproc_func(a IN out integer) returns integer as
$$
declare
  mds integer;
  original integer;
  function square(original in out integer) return integer;
  function square(original in out integer) return integer
  AS
  declare
       original_squared integer;
  begin
       original_squared := original * original;
       original := original_squared + 1;
       return original_squared;
   end;
begin
    mds := 10;
    original := square(mds);
    raise info '%',original;
    a := original + 1;
    return mds;
end;$$ language plisql;
/

-- pg_get_functiondef
SELECT pg_get_functiondef('bug139_func'::regproc) from dual;
SELECT pg_get_functiondef('bug139_proc'::regproc) from dual;
SELECT pg_get_functiondef('test_subproc_func'::regproc) from dual;

-- ivy_get_plisql_functiondef is only using get plisql func/proc definition.
SELECT ivy_get_plisql_functiondef('bug139_func'::regproc) from dual;
SELECT ivy_get_plisql_functiondef('bug139_proc'::regproc) from dual;
SELECT ivy_get_plisql_functiondef('test_subproc_func'::regproc) from dual;

DROP PROCEDURE bug139_proc();
DROP FUNCTION bug139_func();
DROP FUNCTION test_subproc_func(int);
/* End - BUG:Z139 */

/* Begin - BUG:M0000068 */
-- specify prointisnumber with CREATE FUNCTION
create or replace function test_prointisnumber(id integer) return integer
int is number
is
 var1 integer;
begin
  raise notice '%', id;
  var1 := id;
  raise notice '%', var1;
  var1 := 1.1;
  raise notice '%', var1;
  return id;
end;
/

select proname, prointisnumber from pg_proc where proname = 'test_prointisnumber';

-- specify prointisnumber depend on GUC ivorysql.convert_plisql_int_to_number
set ivorysql.convert_plisql_int_to_number to off;
create or replace function test_prointisnumber(id integer) return integer
is
 var1 integer;
begin
  raise notice '%', id;
  var1 := id;
  raise notice '%', var1;
  var1 := 1.1;
  raise notice '%', var1;
  return id;
end;
/

select proname, prointisnumber from pg_proc where proname = 'test_prointisnumber';

set ivorysql.convert_plisql_int_to_number to on;
create or replace function test_prointisnumber(id integer) return integer
is
 var1 integer;
begin
  raise notice '%', id;
  var1 := id;
  raise notice '%', var1;
  var1 := 1.1;
  raise notice '%', var1;
  return id;
end;
/

select proname, prointisnumber from pg_proc where proname = 'test_prointisnumber';

-- test call
set ivorysql.convert_plisql_int_to_number to off;
create or replace function test_f(id integer) return integer is
 var1 integer;
begin
  raise notice '%', id;
  var1 := id;
  raise notice '%', var1;
  var1 := 1.1;
  raise notice '%', var1;
  return id;
end;
/

select test_f(1) from dual;		-- ok
select test_f(3.14) from dual;	-- error

alter function test_f(id integer) int is number;
select test_f(1) from dual;		-- ok
select test_f(3.14) from dual;	-- ok

set ivorysql.convert_plisql_int_to_number to on;
create or replace function test_f1(id smallint) return smallint is
 var1 smallint;
begin
  raise notice '%', id;
  var1 := id;
  raise notice '%', var1;
  var1 := 1.1;
  raise notice '%', var1;
  return id;
end;
/

select test_f1(1) from dual;	-- ok
select test_f1(3.14) from dual;	-- ok

-- get definition display integer datatype still
select pg_get_function_arguments('test_f'::regproc);
select pg_get_function_identity_arguments('test_f'::regproc);
select pg_get_function_result('test_f'::regproc);
select pg_get_functiondef('test_f'::regproc);
reset ivorysql.convert_plisql_int_to_number;

-- OUT parameters of function
create or replace function test_f2(id integer, OUT z integer, OUT y int) return int is
 var1 integer;
begin
  raise notice '%', id;
  var1 := id;
  raise notice '%', var1;
  var1 := 1.1;
  raise notice '%', var1;
  z := id;
  raise notice '%', z;
  y := id;
  return id;
end;
/

alter function test_f2(id integer, OUT z integer, OUT y int) int is number;

DECLARE
 a int;
 b int;
 c int;
BEGIN
  c := test_f2(3.14, a, b);
  raise notice '%', a;
END;
/

-- TEST FOR PROCEDURES
CREATE PROCEDURE test_p(IN a int, OUT b int)
IS
BEGIN
  RAISE NOTICE 'a: %, b: %', a, b;
  b := a * 2;
END;
/

-- ok
DECLARE
 b int;
BEGIN
  call test_p(3, b);
  RAISE NOTICE 'b: %', b;
END;
/

-- error
DECLARE
 b int;
BEGIN
  call test_p(3.14, b);
  RAISE NOTICE 'b: %', b;
END;
/

alter procedure test_p(IN a int, OUT b int) int is number;

--ok
DECLARE
 b int;
BEGIN
  call test_p(3, b);
  RAISE NOTICE 'b: %', b;
END;
/

--ok
DECLARE
 b int;
BEGIN
  call test_p(3.14, b);
  RAISE NOTICE 'b: %', b;
END;
/

set ivorysql.convert_plisql_anonymous_int_to_number to on;
alter procedure test_p(IN a int, OUT b int) int is number;	-- recompile, make chache invalid.

-- ok
DECLARE
 b int;
BEGIN
  call test_p(3, b);
  RAISE NOTICE 'b: %', b;
END;
/

-- ok
DECLARE
 b int;
BEGIN
  call test_p(3.14, b);
  RAISE NOTICE 'b: %', b;
END;
/

-- default value parameters
set ivorysql.convert_plisql_int_to_number to off;
create or replace function test_f3(id integer, def integer default 3.14) return integer is
 var1 integer;
begin
  raise notice '%', id;
  var1 := id;
  raise notice '%', var1;
  var1 := 1.1;
  raise notice '%', var1;
  raise notice '%', def;
  return id;
end;
/

select test_f3(1) from dual;	-- ok
select test_f3(3.14) from dual;	-- error

alter function test_f3(id integer, def integer) int is number;
select test_f3(1) from dual;	-- ok
select test_f3(3.14) from dual;	-- ok

-- nested subproc
set ivorysql.convert_plisql_anonymous_int_to_number to on;
DECLARE
  FUNCTION square (original int)
    RETURN int
  AS
    original_squared int;
  BEGIN
    original_squared := original * original;
	raise notice '%', original_squared;
    RETURN original;
  END;
BEGIN
  raise notice '%', square(3.14);
END;
/

reset ivorysql.convert_plisql_anonymous_int_to_number;

create or replace function test_subproc_func(a in integer) return integer
is
  mds integer;
  function square(original in integer) return integer;
  function square(original in integer) return integer
  is
  declare
       original_squared integer;
  begin
	   raise info 'subproc: %',original;
       original_squared := original * original;
	   raise info 'subporc: %',original_squared;
       return original;
   end;
begin
	raise info 'parent: %',square(a);
	raise info 'parent: %',square(3.14);
    return a;
end;
/

select test_subproc_func(3) from dual;		-- error
select test_subproc_func(3.14) from dual;	-- error
alter function test_subproc_func(a in integer) int is number;
select test_subproc_func(3) from dual;		-- ok
select test_subproc_func(3.14) from dual;	-- ok

create or replace procedure test_subproc_proc(a integer)
is
  mds integer;
  function square(original in integer) return integer;
  function square(original in integer) return integer
  is
  declare
       original_squared integer;
  begin
	   raise info 'subproc: %',original;
       original_squared := original * original;
	   raise info 'subporc: %',original_squared;
       return original;
   end;
begin
	raise info 'parent: %',square(a);
	raise info 'parent: %',square(3.14);
end;
/

call test_subproc_proc(3);		-- error
call test_subproc_proc(3.14);	-- error
alter procedure test_subproc_proc int is number;
call test_subproc_proc(3);		-- ok
call test_subproc_proc(3.14);	-- ok

-- clean
drop function test_prointisnumber(id integer);
drop function test_f(id integer);
drop function test_f1(id smallint);
drop function test_f3(id integer, def integer);
drop function test_f2(id integer, OUT z integer, OUT y int);
drop procedure test_p(IN a int, OUT b int);
drop function test_subproc_func(a in integer);
drop procedure test_subproc_proc(a integer);
/* End - BUG:M0000068 */

/* Begin - BUG:Z227 */
CREATE OR REPLACE FUNCTION f_def(arg1 int := 1)
RETURN int
IS
BEGIN
   RETURN arg1;
END;
/

CREATE OR REPLACE PROCEDURE p_def(arg1 int := 1)
IS
BEGIN
   raise info 'default val: %x', arg1;
END;
/

DROP FUNCTION f_def;
DROP PROCEDURE p_def;
/* End - BUG:Z227 */

/* Begin - BUG:M0000079 */
-- do stmt
declare
var char(3);
begin
var = '12345';
end;
/

do
$$
declare
var char(3);
begin
var = '12345';
end;
$$ language plpgsql;

-- function
create or replace function ora_fbug79(arg char(3)) return int is
 var char(3);
begin
  raise notice '%', arg;
  var := arg;
  raise notice '%', var;
  var := '12345';
  return 0;
end;
/

create or replace function pg_fbug79(arg char(3)) return int as
$$
declare
 var char(3);
begin
  raise notice '%', arg;
  var := arg;
  raise notice '%', var;
  var := '12345';
  return 0;
end;
$$ language plpgsql;
/

select ora_fbug79('abc');
select pg_fbug79('abc');

-- procedure
create or replace procedure ora_pbug79(arg char(3)) is
 var char(3);
begin
  raise notice '%', arg;
  var := arg;
  raise notice '%', var;
  var := '12345';
end;
/

create or replace procedure pg_pbug79(arg char(3)) as
$$
declare
 var char(3);
begin
  raise notice '%', arg;
  var := arg;
  raise notice '%', var;
  var := '12345';
end;
$$ language plpgsql;
/

call ora_pbug79('abc');
call pg_pbug79('abc');

drop function ora_fbug79;
drop function pg_fbug79;
drop procedure ora_pbug79;
drop procedure pg_pbug79;
/* End - BUG:M0000079 */

/* Begin - ReqID:SRS-SQL-CALL */
-- An empty parenthesis cannot be ignored
create or replace procedure p_noarg()
is
begin
raise notice 'this procedure without args';
end;
/

-- fail
call p_noarg;

--ok
call p_noarg();

create or replace function f_noarg()
return number
is
begin
raise notice 'this function without args';
return 999;
end;
/

variable x number;

-- fail
call f_noarg into :x;

--ok
call f_noarg() into :x;
print x

-- function or procedure with default value
create or replace procedure p_defs(a varchar2 default 'i am a default string')
is
begin
raise notice '%', a;
end;
/

-- fail
call p_defs;

-- ok
call p_defs();
--ok
call p_defs('new string');

create or replace function f_defs(a number default 1314)
return number
is
begin
raise notice '%', a;
return a;
end;
/

variable x number

--fail
call f_defs into :x;

--ok
call f_defs() into :x;
print x

--ok
call f_defs(999) into :x;
print x

drop function f_defs;
drop procedure p_defs;
drop function f_noarg();
drop procedure p_noarg();

--
-- Truncate when the bind variable and the function return type or param type
-- are the same and the typmod of the bind variable overflows.
-- Fails when the bind variable and the function return type are or param type
-- not same but can cast and the typmod of the bind variable overflows.
--
create or replace procedure ff_proc(a varchar2, b varchar2)
is
begin
raise notice '%', a;
raise notice '%', b;
raise notice '%', a||b;
end;
/

-- ok
call ff_proc('123456789', '123456789');

create or replace function ff_test(a varchar2, b varchar2)
return varchar2
is
begin
raise notice '%', a;
raise notice '%', b;
return a||b;
end;
/

-- ok
variable x varchar2(10 char);
call ff_test('123456789', '123456789') into :x;
print x

-- ok
variable x varchar2(10 byte);
call ff_test('123456789', '123456789') into :x;
print x

-- error
variable x char(10 char);
call ff_test('123456789', '123456789') into :x;
print x

-- error
variable x char(10 byte);
call ff_test('123456789', '123456789') into :x;
print x

-- ok
variable x number;
call ff_test('123456789', '123456789') into :x;
print x

-- error
variable x number;
call ff_test('abc', '123456789') into :x;
print x

create or replace procedure f_proc(a varchar2, b out varchar2)
is
begin
raise notice '%', a;
raise notice '%', b;
b := a;
end;
/

-- ok
variable x varchar2(5 char)
call f_proc('123456789', :x);
print

-- ok
variable x varchar2(5 byte)
call f_proc('123456789', :x);
print

-- error
variable x char(5 char)
call f_proc('123456789', :x);
print

-- error
variable x char(5 byte)
call f_proc('123456789', :x);
print

create or replace function f_test(a varchar2, b out varchar2)
return varchar2
is
begin
raise notice '%', a;
raise notice '%', b;
b := a;
raise notice '%', a;
raise notice '%', b;
return a||b;
end;
/

-- ok
variable x varchar2(10) = '123456789'
variable y varchar2(11)
variable z varchar2(12 char)
call f_test(:x, :y) INTO :z;
print x y z

-- ok
variable x varchar2(10) = '123456789'
variable y varchar2(11)
variable z varchar2(12 byte)
call f_test(:x, :y) INTO :z;
print x y z

-- ok
variable x varchar2(10) = '123456789'
variable y varchar2(5 char)
variable z varchar2(20)
call f_test(:x, :y) INTO :z;
print x y z

-- ok
variable x varchar2(10) = '123456789'
variable y varchar2(5 byte)
variable z varchar2(20)
call f_test(:x, :y) INTO :z;
print x y z

-- error
variable x varchar2(10) = '123456789'
variable y varchar2(11)
variable z varchar2(12 char)

begin
:z := f_test(:x, :y);
end;
/
print x y z

-- error
variable x char(10) = '123456789'
variable y char(11)
variable z char(12 char)
call f_test(:x, :y) INTO :z;
print x y z

-- error
variable x char(10) = '123456789'
variable y char(11)
variable z char(12 byte)
call f_test(:x, :y) INTO :z;
print x y z

-- ok
variable x char(10) = '123456789'
variable y char(11)
variable z char(20 char)
call f_test(:x, :y) INTO :z;
print x y z

-- ok
variable x char(10) = '123456789'
variable y char(11)
variable z char(20 byte)
call f_test(:x, :y) INTO :z;
print x y z

-- error
variable x char(10) = '123456789'
variable y char(5 char)
variable z char(20)
call f_test(:x, :y) INTO :z;
print x y z

-- error
variable x char(10) = '123456789'
variable y char(5 byte)
variable z char(20)
call f_test(:x, :y) INTO :z;
print x y z

-- ok
variable x char(10) = '123456789'
variable y char(10 char)
variable z char(20)
call f_test(:x, :y) INTO :z;
print x y z

-- ok
variable x char(10) = '123456789'
variable y char(10 byte)
variable z char(20)
call f_test(:x, :y) INTO :z;
print x y z

-- ORA-06578: output parameter cannot be a duplicate bind.
-- Cause: The bind variable corresponding to an IN/OUT or
-- OUT parameter for a function or a procedure or a function
-- return value in a CALL statement cannot be a duplicate bind variable.
call f_test(:x, :x) INTO :z;
call f_proc(:x, :x);

--clean
drop procedure ff_proc;
drop procedure f_proc;
drop function f_test;
drop function ff_test;

-- Use the CALL statement to execute a procedure or function defined within package.
create or replace package call_pkg is
  function f_test(a varchar2, b out varchar2, c out number) return varchar2;
  procedure p_test(a varchar2, b out varchar2, c out number);
end;
/

create or replace package body call_pkg is
function f_test(a varchar2, b out varchar2, c out number) return varchar2 is
begin
	raise notice '%', a;
	raise notice '%', b;
	b := a;
	c := 1314;
	raise notice '%', a;
	raise notice '%', b;
	return a||b;
end;
procedure p_test(a varchar2, b out varchar2, c out number) is
begin
	raise notice '%', a;
	raise notice '%', b;
	b := a;
	c := 1314;
end;
end;
/

variable x varchar2(10) = '123456789'
variable y varchar2(11)
variable z number
variable r varchar2(12 char)
call call_pkg.f_test(:x, :y, :z) INTO :r;
print x y z r

variable x varchar2(10) = '123456789'
variable y varchar2(11)
variable z number
call call_pkg.p_test(:x, :y, :z);
print x y z r

drop package call_pkg;

/* End - ReqID:SRS-SQL-CALL */

--bug#M0000136
create table test_insert(id integer,name varchar2(256));

--test if stmt
declare
  var1 test_insert%rowtype;
begin
  var1.id := 2;
  var1.name := 'ok';
  if var1.id = 2 then
      begin
       execute 'insert into test_insert values(:1,:2)' using var1.id,var1.name;
      end;
  end if;
end;
/

declare
  var1 test_insert%rowtype;
begin
  var1.id := 3;
  var1.name := 'ok';
  if var1.id = 2 then
      begin
       execute 'insert into test_insert values(:1,:2)' using var1.id,var1.name;
      end;
  elsif var1.id = 3 THEN
      begin
       execute 'insert into test_insert values(:1,:2)' using var1.id,var1.name;
      end;
  end if;
end;
/


declare
  var1 test_insert%rowtype;
begin
  var1.id := 4;
  var1.name := 'ok';
  if var1.id = 2 then
      begin
       execute 'insert into test_insert values(:1,:2)' using var1.id,var1.name;
      end;
  elsif var1.id = 3 THEN
      begin
       execute 'insert into test_insert values(:1,:2)' using var1.id,var1.name;
      end;
  ELSE
     begin
       execute 'insert into test_insert values(:1,:2)' using var1.id,var1.name;
      end;
  end if;
end;
/


--test case stmt
DECLARE
  var1 test_insert%rowtype;
BEGIN
  var1.id := 5;
  var1.name := 'ok1';
  CASE var1.id
    WHEN 5 THEN
       begin
       execute 'insert into test_insert values(:1,:2)' using var1.id,var1.name;
      end;
	WHEN 6 THEN
	  begin
       execute 'insert into test_insert values(:1,:2)' using var1.id,var1.name;
      end;
    else
      begin
       execute 'insert into test_insert values(:1,:2)' using var1.id,var1.name;
      end;
	END case;
end;
/

DECLARE
  var1 test_insert%rowtype;
BEGIN
  var1.id := 6;
  var1.name := 'ok1';
  CASE var1.id
    WHEN 5 THEN
       begin
       execute 'insert into test_insert values(:1,:2)' using var1.id,var1.name;
      end;
	WHEN 6 THEN
	  begin
       execute 'insert into test_insert values(:1,:2)' using var1.id,var1.name;
      end;
    else
      begin
       execute 'insert into test_insert values(:1,:2)' using var1.id,var1.name;
      end;
	END case;
end;
/

DECLARE
  var1 test_insert%rowtype;
BEGIN
  var1.id := 7;
  var1.name := 'ok1';
  CASE var1.id
    WHEN 5 THEN
       begin
       execute 'insert into test_insert values(:1,:2)' using var1.id,var1.name;
      end;
	WHEN 6 THEN
	  begin
       execute 'insert into test_insert values(:1,:2)' using var1.id,var1.name;
      end;
    else
      begin
       execute 'insert into test_insert values(:1,:2)' using var1.id,var1.name;
      end;
	END case;
end;
/

--test while
declare
  var1 test_insert%rowtype;
begin
  var1.id := 8;
  var1.name := 'name';
  while true loop
      begin
       execute 'insert into test_insert values(:1,:2)' using var1.id,var1.name;
      end;
      exit;
  end loop;
end;
/

--test loop
declare
  var1 test_insert%rowtype;
begin
  var1.id := 9;
  var1.name := 'name';
  loop
      begin
       execute 'insert into test_insert values(:1,:2)' using var1.id,var1.name;
      end;
      exit;
  end loop;
end;
/

--test fori
declare
  var1 test_insert%rowtype;
begin
  var1.id := 10;
  var1.name := 'name';
  FOR i IN 1.. 2 loop
      begin
       execute 'insert into test_insert values(:1,:2)' using var1.id,var1.name;
      end;
      exit;
  end loop;
end;
/


--test forc
declare
  var1 test_insert%rowtype;
  c CURSOR IS SELECT * FROM test_insert ORDER BY id;
begin
  var1.id := 11;
  var1.name := 'name';
  FOR i IN c LOOP
      begin
       execute 'insert into test_insert values(:1,:2)' using var1.id,var1.name;
      end;
      exit;
  end loop;
end;
/

--test fors
declare
  var1 test_insert%rowtype;
  var3 test_insert%rowtype;
begin
  var1.id := 12;
  var1.name := 'name';
  FOR var3 IN EXECUTE 'SELECT * FROM test_insert order by id' LOOP
      begin
       execute 'insert into test_insert values(:1,:2)' using var1.id,var1.name;
      end;
      exit;
  end loop;
end;
/

SELECT * FROM test_insert ORDER BY id;

declare
  var1 test_insert%rowtype;
  var2 integer;
begin
  var1.id := 2;
  var2 := 10 / 0;
exception
  when others then
    begin
      execute 'insert into test_insert values(:1,:2)' using var1.id,var1.name;
    end;
end;
/

--/* Begin - BUG#M0000292: ? As a binding variable placeholder, support =?, >=? and similar operators */
declare
  v1 varchar(10);
begin
  select relname into v1 from pg_class where relnamespace = ? and oid=16384;
end;
/

declare
  v1 varchar(10);
begin
  select relname into v1 from pg_class where relnamespace=? and oid=16384;
end;
/

declare
  v1 varchar(10);
begin
  select relname into v1 from pg_class where relnamespace>=? and oid=16384;
end;
/
--/* End - BUG#M0000292: ? As a binding variable placeholder, support =?, >=? and similar operators */

--clean data
DROP TABLE test_insert;

/* Begin - BUG #M0000304 */
CREATE TABLE test1 (a int);
CREATE OR REPLACE PROCEDURE protest2
 AS
 BEGIN
INSERT INTO test1 VALUES (1);
 END;
 /

call protest2();
exec protest2();
exec protest2;

BEGIN
protest2();
END;
/

BEGIN
protest2;
END;
/

--error
BEGIN
exec protest2();
END;
/

BEGIN
call protest2();
END;
/

CREATE or replace FUNCTION  func2
RETURN INT
AS
BEGIN
protest2();
RETURN 25;
END;
/

select func2() from dual;

select * from test1;
truncate test1;

CREATE OR REPLACE PROCEDURE exec
AS
BEGIN
INSERT INTO test1 VALUES (2);
END;
/

call exec();
exec exec();
exec exec;

BEGIN
exec;
END;
/

BEGIN
exec();
END;
/

select * from test1;

DROP TABLE test1;

CREATE TABLE test1 (a int);
CREATE PROCEDURE test_proc3(x int)
LANGUAGE plisql
AS $$
BEGIN
    INSERT INTO test1 VALUES (x);
END;
$$;
/

call test_proc3(1);
exec test_proc3(2);

BEGIN
test_proc3(3);
END;
/

select * from test1;


CREATE PROCEDURE test_proc4(y int)
LANGUAGE plisql
AS $$
BEGIN
     test_proc3(y);
END;
$$;
/

call test_proc4(4);
exec test_proc4(5);

BEGIN
test_proc4(6);
END;
/

select * from test1;

begin
CREATE TEMP TABLE __rp_ora_alerts(event text, message text);
end;
/

do
$$
	begin
		analyze;
	end;
$$ language plisql;

do
$$
	begin
		call;
	end;
$$ language plisql;



DROP TABLE test1;
DROP TABLE __rp_ora_alerts;
/*End - BUG #M0000304*/

/* Begin - BUG#M0000376 */

set ivorysql.enable_get_oraparam_mode = true;

create table t1(id int, name varchar(20));
insert into t1 values(1, 'a');
insert into t1 values(2, 'b');

--assign statment
select get_parameter_descr('do $$ BEGIN insert into t1 values(:x, :y) ; END; $$ using :x inout, :y inout PARAMSTYPE 23, 25;');
select get_parameter_descr('do $$ BEGIN :x := 23; END; $$ using :x inout PARAMSTYPE 23;');
select get_parameter_descr('do $$ BEGIN :x := :y; END; $$ using :x inout,:y inout  PARAMSTYPE 23, 23;');
select get_parameter_descr('do $$ BEGIN :x := 23 + :x; END; $$ using :x inout  PARAMSTYPE 23;');
select get_parameter_descr('do $$ BEGIN :x := 23; :y := :x; END; $$ using :x inout, :y inout  PARAMSTYPE 23, 23;');

--select into parameters
select get_parameter_descr('do $$ BEGIN select id into :x from t1 ; END; $$ using :x inout PARAMSTYPE 23;');
select get_parameter_descr('do $$ DECLARE a int; BEGIN select id into :x from t1 ; a = :x; END; $$ using :x inout PARAMSTYPE 23;');

--return into parameters
select get_parameter_descr('do $$ BEGIN delete from t1 returning id into :x; END; $$ using :x inout PARAMSTYPE 23;');
select get_parameter_descr('do $$ DECLARE a int; BEGIN delete from t1 returning id into :x; a = :x; END; $$ using :x inout PARAMSTYPE 23;');

--fetch into parameters
select * from get_parameter_descr('
 do $$
 declare
   l_name varchar(256);
   l_num integer;
   cursor c1 is select * from t1;
 begin
   open c1;
   LOOP
     fetch c1 into :1, :2;
     exit when c1%NOTFOUND;
   end LOOP;
   close c1;
 end; $$ using :1 inout, :2 inout PARAMSTYPE 23, 25');

select * from get_parameter_descr('
 do $$
 declare
   l_name varchar(256);
   l_num integer;
   cursor c1 is select * from t1;
 begin
   open c1;
   LOOP
     fetch c1 into :1, :2;
	 l_name := :2;
	 l_num := :1;
     exit when c1%NOTFOUND; 
   end LOOP;
   close c1;
 end; $$ using :1 inout, :2 inout PARAMSTYPE 23, 25');
 
--mix statments
select * from get_parameter_descr('
 do $$
 declare
	id integer;
	salary integer;
	id1 integer;
	id2 varchar(256);
begin
	insert into t1(id, name) values(:1,:2);
	id := :1;
	:1 := id * 2 + 1;
	:x := ''this is a test ok'';
	:y := 23;
end; $$ using :1 inout, :2 in, :x out, :y out PARAMSTYPE 23, 23, 25, 23');

create or replace function f1(id in int, id2 out int) return int as
begin
  id2 := id;
  return 1;
end;
/
create or replace function f2(id in int, id2 inout int) return int as
begin
  id2 := id;
  return 2;
end;
/
create or replace function f3(id in int, name text default 'a', id2 inout int) return int as
begin
  id2 := id;
  return 3;
end;
/
create or replace procedure p1(id in int, name text default 'a', id2 inout int)  as
begin
  id2 := id;
end;
/

--call a proc function or procedure
select get_parameter_descr('do $$ declare a int := 11;  b int; BEGIN insert into t1 values(f1(a, b),:x) ; END; $$ using :x inout PARAMSTYPE 25;');

select get_parameter_descr('do $$ declare a int := 11;  b int; BEGIN b := f1(a,:x); END; $$ using :x inout PARAMSTYPE 23;');
select get_parameter_descr('do $$ declare b int; BEGIN b := f1(:y,:x); END; $$ using :y inout, :x inout PARAMSTYPE 23, 23;');
select get_parameter_descr('do $$ declare b int; BEGIN b := f1(id2=>:y,id=>:x); END; $$ using :y inout, :x inout PARAMSTYPE 23, 23;');
select get_parameter_descr('do $$ declare b int; BEGIN b := f1(id2=>:y,id=>:x+:z); END; $$ using :y inout, :x inout,:z inout PARAMSTYPE 23, 23 ,23;');

select get_parameter_descr('do $$ declare a int; b int := 2;  BEGIN b := f1(1, id2=>:x); END; $$ using :x inout PARAMSTYPE 23;');
select get_parameter_descr('do $$ declare a int; b int := 2;  BEGIN b := f1(1, id2=>:x); :x = 1; END; $$ using  :x inout PARAMSTYPE 23;');

select get_parameter_descr('do $$ declare a int; b int := 2;  BEGIN CALL p1(id2=>:x, id=>:y); END; $$ using :x inout,:y inout PARAMSTYPE 23,23;');
select get_parameter_descr('do $$ declare a int; b int := 2;  BEGIN CALL p1(id2=>:x, id=>:y); :y = 1; END; $$ using  :x inout,:y inout PARAMSTYPE 23,23;');

select get_parameter_descr('do $$   BEGIN  delete from t1 where id = f2(id2=>:y, id=>:x); END; $$ using :y inout,:x inout PARAMSTYPE 23,23;');
select get_parameter_descr('do $$   BEGIN  delete from t1 where id = f2(id2=>:y, id=>:x); :x = 1; END; $$ using  :y inout,:x inout PARAMSTYPE 23,23;');

select get_parameter_descr('do $$ declare a int := 11;  b int; BEGIN b := f2(a,:x); END; $$ using :x inout PARAMSTYPE 23;');
select get_parameter_descr('do $$ declare b int; BEGIN b := f3(id2=>:y,id=>:x); END; $$ using :y inout, :x inout PARAMSTYPE 23, 23;');
select get_parameter_descr('do $$ declare b int; BEGIN b := f3(name=> :1, id2=>:y,id=>:x); END; $$ using :1 inout, :y inout, :x inout PARAMSTYPE 25, 23, 23;');

--call a package function or procedure
create or replace package pkg as
  function f1(id in integer, id2 out integer) return integer;
  function f2(id in integer, id2 in out integer) return integer;
  function f3(id out integer, name text default 'a') return integer;
  procedure p1(id in integer, name text default 'a', name2 in out varchar);
end;
/

create or replace package body pkg as
  function f1(id in integer, id2 out integer) return integer as
  begin
    id2 := 1;
	return 1;
  end;
  function f2(id in integer, id2 in out integer) return integer as
  begin
    id2 := 2;
	return 2;
  end;
  function f3(id out integer, name text default 'a') return integer as
  begin
    id := 3;
	return 3;
  end;
  procedure p1(id in integer, name text default 'a', name2 in out varchar) as
  begin
    name2 := 'procedure1';
  end;
end;
/
select get_parameter_descr('do $$ declare b int; BEGIN b := pkg.f1(:1,:x); END; $$ using :1 inout, :x inout PARAMSTYPE 23, 23;');
select get_parameter_descr('do $$ declare b int; BEGIN b := pkg.f2(id2=>:y,id=>:x); END; $$ using :y inout, :x inout PARAMSTYPE 23, 23;');
select get_parameter_descr('do $$ declare b int; BEGIN b := pkg.f3(:x); END; $$ using :x inout PARAMSTYPE 23;');
select get_parameter_descr('do $$ declare b int; BEGIN b := pkg.f3(name=>:x, id=>:y); END; $$ using :x inout, :y inout PARAMSTYPE 25, 23;');
select get_parameter_descr(
  'do $$
  declare
    b int;
  BEGIN
    CALL pkg.p1(name2=>:x, id=>:y, name=>:z);
  END; $$ using :x inout, :y inout, :z inout PARAMSTYPE 25, 23, 25;
');


--not an anonymous block, can't get parameters mode
select get_parameter_descr('select f1(1,id2=>:x)');

--nested proc function call
select * from get_parameter_descr('
do $$
declare
  id integer;
begin
  id := f1(f2(:x, :y), :z);
  end; $$ using  :x in, :y out, :z inout PARAMSTYPE 23, 23, 23');
  
select * from get_parameter_descr('
do $$
declare
  id integer;
begin
  id := f1(:z + f2(:x, :y), :k);
end; $$ using :z in,  :x in, :y inout, :k out PARAMSTYPE 23, 23, 23, 23');

select * from get_parameter_descr('
do $$
declare
  id integer;
begin
  id := f1(:y + f2(:x, :z), :y);
end; $$ using  :y inout, :x in, :z inout PARAMSTYPE 23, 23, 23');

select * from get_parameter_descr('
do $$
declare
  id integer;
begin
  id := f3(f1(:z + f2(:x, :1), :y), :2, :3);
end; $$ using :z in,  :x in, :1 out, :y inout, :2 in, :3 inout PARAMSTYPE 23, 23, 23, 23, 25, 23');

select * from get_parameter_descr('
do $$
declare
begin
  p1(f1(:z + f2(:x, :1), :y), :2, :3);
end; $$ using :z in,  :x in, :1 out, :y inout, :2 in, :3 inout PARAMSTYPE 23, 23, 23, 23, 25, 23');

select * from get_parameter_descr('
do $$
declare
begin
  CALL p1(f1(:z + f2(:x, :1), :y), :2, :3);
end; $$ using :z in,  :x in, :1 out, :y inout, :2 in, :3 inout PARAMSTYPE 23, 23, 23, 23, 25, 23');


--nested package function call
drop package pkg;
create or replace package pkg as
  function f1(id in integer, id2 out integer) return integer;
  function f2(id in integer, id2 in out integer) return integer;
  function f3(id in int, name text default 'a', id2 in out int) return integer;
  procedure p1(id in integer, name text default 'a', name2 in out varchar);
end;
/
create or replace package body pkg as
  function f1(id in integer, id2 out integer) return integer as
  begin
    id2 := 1;
	return 1;
  end;
  function f2(id in integer, id2 in out integer) return integer as
  begin
    id2 := 2;
	return 2;
  end;
  function f3(id in int, name text default 'a', id2 in out int) return integer as
  begin
    id2 := 3;
	return 3;
  end;
  procedure p1(id in integer, name text default 'a', name2 in out varchar) as
  begin
    name2 := 'procedure1';
  end;
end;
/

select get_parameter_descr(
'do $$
declare
  id integer;
begin
  id := pkg.f1(pkg.f2(:x, :y), :z);
  end; $$ using  :x in, :y out, :z inout PARAMSTYPE 23, 23, 23;
');

select * from get_parameter_descr('
do $$
declare
  id integer;
begin
  id := pkg.f1(:z + pkg.f2(:x, :y), :k);
end; $$ using :z in,  :x in, :y inout, :k out PARAMSTYPE 23, 23, 23, 23');

select * from get_parameter_descr('
do $$
declare
  id integer;
begin
  id := pkg.f1(:y + pkg.f2(:x, :z), :y);
end; $$ using  :y inout, :x in, :z inout PARAMSTYPE 23, 23, 23');

select * from get_parameter_descr('
do $$
declare
  id integer;
begin
  id := pkg.f3(pkg.f1(:z + pkg.f2(:x, :1), :y), :2, :3);
end; $$ using :z in,  :x in, :1 out, :y inout, :2 in, :3 inout PARAMSTYPE 23, 23, 23, 23, 25, 23');

select * from get_parameter_descr('
do $$
declare
begin
  pkg.p1(pkg.f1(:z + pkg.f2(:x, :1), :y), :2, :3);
end; $$ using :z in,  :x in, :1 out, :y inout, :2 in, :3 inout PARAMSTYPE 23, 23, 23, 23, 25, 23');


select * from get_parameter_descr('
do $$
declare
  id integer;
begin
  id := pkg.f3(f1(:z + pkg.f2(:x, :1), :y), :2, :3);
end; $$ using :z in,  :x in, :1 out, :y inout, :2 in, :3 inout PARAMSTYPE 23, 23, 23, 23, 25, 23');

select * from get_parameter_descr('
do $$
declare
  id integer;
begin
  pkg.p1(f1(:z + pkg.f2(:x, :1), :y), :2, :3);
end; $$ using :z in,  :x in, :1 out, :y inout, :2 in, :3 inout PARAMSTYPE 23, 23, 23, 23, 25, 23');

select * from get_parameter_descr('
do $$
declare
  id integer;
begin
  CALL pkg.p1(f1(:z + pkg.f2(:x, :1), :y), :2, :3);
end; $$ using :z in,  :x in, :1 out, :y inout, :2 in, :3 inout PARAMSTYPE 23, 23, 23, 23, 25, 23');

--nested inline function call
select * from get_parameter_descr('
do $$
declare
  id integer;
  function f1(i int, j out int) return int as
  declare
  begin
    j := 100;
    return i;
  end;
  function f2(id in integer, id2 in out integer) return integer as
  begin
    id2 := 2;
    return 2;
  end;
  function f3(id in int, name text default ''a'', id2 in out int) return integer as
  begin
    id2 := 3;
    return 3;
  end;
begin
	id := f1(:x, :y);
	id := f3(f1(:1 + f2(:2, :3), :4), :5, :6);
	id := f1(:7, :7);
end; $$ using  :x in, :y out, :1 in, :2 in, :3 inout, :4 out, :5 in, :6 inout, :7 inout PARAMSTYPE 23, 23, 23, 23, 23, 23, 25, 23, 23');

select * from get_parameter_descr('
do $$
declare
  id integer;
  function f1(i int, j out int) return int as
  declare
  begin
    j := 100;
    return i;
  end;
  procedure p1(id in integer, name text default ''a'', name2 in out varchar) as
  begin
    name2 := ''procedure1'';
  end;
begin
  p1(f1(:x, :y), :2, :3);
end; $$ using  :x in, :y out, :2 in, :3 inout PARAMSTYPE 23, 23, 25, 23');

select * from get_parameter_descr('
do $$
declare
  id integer;
  function f1(i int, j out int) return int as
  declare
  begin
    j := 100;
    return i;
  end;
  procedure p1(id in integer, name text default ''a'', name2 in out varchar) as
  begin
    name2 := ''procedure1'';
  end;
begin
  CALL p1(f1(:x, :y), :2, :3);
end; $$ using  :x in, :y out, :2 in, :3 inout PARAMSTYPE 23, 23, 25, 23');



drop function f1(id in int, id2 out int);
drop function f2(id in int, id2 inout int);
drop function f3(id in int, name text, id2 inout int);
drop procedure p1(id in int, name text, id2 inout int);
drop table t1;
drop package pkg;

reset ivorysql.enable_get_oraparam_mode;
/* End - BUG#M0000376 */

/* Begin - BUG#M0000409 */
create table t_bug409(id int ,id2 int);
set ivorysql.enable_get_oraparam_mode = true;

select * from get_parameter_descr('
 do $$
begin
 insert into t_bug409 values(:node1, :node2);end;$$ using inout, inout PARAMSTYPE 23, 23');
/* End - BUG#M0000409 */

--bug#Z1269
--ok
create or replace function test_f_array(variadic int[]) return integer is
begin
	return $1[1];
end;
/

select test_f_array(1,2) from dual;
select test_f_array(3,4,5) from dual;

--error
create or replace function test_f_array(variadic int[],  integer) return integer is
begin
	return $1[1];
end;
/

--error
create or replace function test_f_array(variadic int[], out integer) return integer is
begin
	return $1[1];
end;
/

--error
create or replace function test_f_array(variadic int[], IN out integer) return integer is
begin
	return $1[1];
end;
/

show ivorysql.compile_plisql_use_owner;

drop function test_f_array;

--bug#M410
create or replace function test_f(id integer) return integer is
/* this is a test
 *commnet
 */
begin
  /* is is a test comment
   a mement
   */
  return 1;
end;
/

select test_f(1) from dual;

select prosrc from pg_proc where proname = 'test_f';

create or replace function test_f1(id integer) return integer is
/* this is a test commnet */
var1 integer;
/* var2 integer */
var2 integer;
begin
  /* is is a test comment */
  return 1;
end;
/

select test_f1(1) from dual;
select prosrc from pg_proc where proname = 'test_f1';


create or replace package test_pkg is
 /*/* test a test */
 var1 integer;
 /* a test_f */
 function test_f(id integer) return integer;
end;
/

create or replace package body test_pkg is /* test a tett */
  /* privie var */
  var2 integer;
  function test_f(id integer) return integer is
  begin
    return var1;
  end;
  /* a privie function */
  function test_p(id integer) return integer is
  begin
    /* /* test a test */
    return var1;
  end;
/* init body */
begin
  var1 := 100;
  var2 := 1;
end;
/

declare
  var1 integer;
begin
  var1 := test_pkg.test_f(23);
  raise info '%',var1;
end;
/

select a.pkgsrc,b.bodysrc from pg_package a left join pg_package_body b
on a.oid = b.pkgoid where pkgname = 'test_pkg';

create table test1(c1 varchar2(256));

CREATE OR REPLACE PROCEDURE testp5 IS
/* comment 1*/
BEGIN
/*comment 2*/
/*comment3*/
/*comment4*/
/*comment5*/
/*comment6*/

/*comment7
*comment8
*comment9
*comment10*/
  INSERT INTO test1 (c1) VALUES ('17');
		begin
			INSERT INTO test1 (c1) VALUES ('11');
	    exception when others then null ;
	end;
EXCEPTION
WHEN OTHERS THEN null;
RAISE;
END testp5;
/
call testp5();

select prosrc from pg_proc where proname = 'testp5';

create or replace function test_ff(id integer) return integer is /* a test */
-- a test comment
 var1 integer;
begin
  return 1;
end;
/

SELECT test_ff(1) FROM dual;

select prosrc from pg_proc where proname = 'test_ff';

create or replace package test_pkg1 is
-- a test comment
 var1 integer;
 --a function
 FUNCTION test_f(id integer) RETURN integer;
end;
/

CREATE OR replace package body test_pkg1 IS
  --a test
  var3 integer;
  FUNCTION test_f(id integer) RETURN integer IS
  BEGIN
   RETURN 1;
  end;
begin
  var3 := 1;
  var1 := 2;
end;
/

BEGIN
  raise info '%', test_pkg1.var1;
end;
/

select a.pkgsrc,b.bodysrc from pg_package a left join pg_package_body b
on a.oid = b.pkgoid where pkgname = 'test_pkg1';


\set SEND_SPACE_ONE_COMMENTS false

create or replace function test_ff1(id integer) return integer is /* a test */
-- a test comment
 var1 integer;
begin
  return 1;
end;
/

SELECT test_ff1(1) FROM dual;

select prosrc from pg_proc where proname = 'test_ff1';

--clean data
drop function test_f;
drop function test_f1;
drop package test_pkg;
DROP package test_pkg1;
drop procedure testp5;
DROP TABLE test1;
DROP FUNCTION test_ff;
DROP FUNCTION test_ff1;


--bug#M411
SET ivorysql.enable_compatible_const_integer_operator = on;

SELECT 1/3 FROM dual;

alter session set NLS_DATE_FORMAT='YYYY-MM-DD hh24:mi:ss';

select to_date('2024-11-9 13:45:36', 'yyyy-mm-dd hh24:mi:ss')
,(to_date('2024-11-9 13:45:36', 'yyyy-mm-dd hh24:mi:ss') +1/24/60/30) FROM dual;

CREATE OR REPLACE FUNCTION test_int(id integer) RETURN integer IS
BEGIN
   RETURN id;
end;
/

SELECT test_int(23 + 24) FROM dual;

DECLARE
  var1 integer;
BEGIN
  var1 := 23 + 24;
  raise info '%', var1;
end;
/

DROP FUNCTION test_int;

reset ivorysql.enable_compatible_const_integer_operator;

select to_date('2024-11-9 13:45:36', 'yyyy-mm-dd hh24:mi:ss')
,(to_date('2024-11-9 13:45:36', 'yyyy-mm-dd hh24:mi:ss') +1/24/60/30) FROM dual;

--bug#Z1452
set ivorysql.enable_compatible_numeric_division_result = true;

select 3::bigint/6::int,3::int/6::int,3::smallint/6::int;
select 3::bigint/6::bigint,3::int/6::bigint,3::smallint/6::bigint;
select 3::bigint/6::smallint,3::int/6::smallint,3::smallint/6::smallint;

SELECT '2' / 4, '2' / '4','2' / 4.0, 2 / '4', 2 / 4 FROM dual;

--error
SELECT 'abc' / '3' FROM dual;

--error
SELECT 'abc' / '3' FROM dual;

--error
SELECT 'abc' / 3 FROM dual;

--error
SELECT 3 / 'abc' FROM dual;

SELECT 1/6 FROM dual;


create table t_insert(id integer,name varchar2(256));

create table t_insert_his(id integer,name varchar2(256));

insert into t_insert values(1,'xiexie');
insert into t_insert_his values(1,'xiexie');
insert into t_insert_his values(2,'history');

SELECT
(SELECT sum(1) FROM T_INSERT) sum,
(SELECT count(1) FROM T_INSERT_HIS ) count,
(SELECT count(1) FROM T_INSERT ) countd,
(SELECT sum(1) FROM T_INSERT)/ (SELECT count(1) FROM T_INSERT_HIS ) "sum/count",
(SELECT count(1) FROM T_INSERT )/ (SELECT count(1) FROM T_INSERT_HIS ) "countd/count"
from dual;


CREATE TABLE test_id(id oid);

INSERT INTO test_id VALUES(1);

DROP TABLE test_id;

create table test_id(id char, ids char);

INSERT INTO test_id VALUES(1,2);
INSERT INTO test_id VALUES(2,3);

SELECT id / 2 FROM test_id ORDER BY id;

SELECT id / ids FROM test_id ORDER BY id;

--error
SELECT id / 2 ,xmin / 2 FROM test_id;

reset ivorysql.enable_compatible_numeric_division_result;

select 3::bigint/6::int,3::int/6::int,3::smallint/6::int;
select 3::bigint/6::bigint,3::int/6::bigint,3::smallint/6::bigint;
select 3::bigint/6::smallint,3::int/6::smallint,3::smallint/6::smallint;


SELECT '2' / 4, '2' / '4','2' / 4.0, 2 / '4', 2 / 4 FROM dual;

--error
SELECT 'abc' / '3' FROM dual;

--error
SELECT 'abc' / 3 FROM dual;

--error
SELECT 3 / 'abc' FROM dual;

SELECT 1/6 FROM dual;

SELECT
(SELECT sum(1) FROM T_INSERT) sum,
(SELECT count(1) FROM T_INSERT_HIS ) count,
(SELECT count(1) FROM T_INSERT ) countd,
(SELECT sum(1) FROM T_INSERT)/ (SELECT count(1) FROM T_INSERT_HIS ) "sum/count",
(SELECT count(1) FROM T_INSERT )/ (SELECT count(1) FROM T_INSERT_HIS ) "countd/count"
from dual;

SELECT id / 2 FROM test_id ORDER BY id;

SELECT id / ids FROM test_id ORDER BY id;

--clean data
drop table t_insert;
drop table t_insert_his;
DROP TABLE test_id;

/* Begin - BUG#M00000453 */
create table t_bug453(id int);

select * from get_parameter_descr('insert into t_bug453 values(1);begin insert into t_bug453 values(2);');
select * from get_parameter_descr('insert into t_bug453 values(1)');

drop table t_bug453;
/* End - BUG#M00000453 */

/* Begin -BUG#Z1670 */
create schema bugz1670;
create table bugz1670.t1(id int);
create table bugz1670.t2(id int);


create or replace function bugz1670.testfunc return void is
declare
	line1 bugz1670.t1%rowtype;
	line2 bugz1670.t2%rowtype;
begin
select * into line1 from bugz1670.t1 limit 1;
select * into line2 from bugz1670.t2 limit 1;
end;
/

drop schema bugz1670;
drop schema bugz1670 cascade;
/* End - BUG#Z1670 */


--bug#Z4225
create table t1 (id int, name varchar(100), age int invisible);

insert into t1 (id, name, age) values (1, 'xiexie', 23);


--ok
declare
  v_t1 t1%rowtype;
begin
  select * into v_t1 from t1;
  raise info 'id :=  %,name := %', v_t1.id ,v_t1.name;
end;
/


--failed
declare
  v_t1 t1%rowtype;
begin
  select id,name,age into v_t1 from t1;
   raise info 'id :=  %,name := %', v_t1.id ,v_t1.name;
end;
/


--failed
declare
  v_t1 t1%rowtype;
  v_t2 integer;
begin
  select id,name,age into v_t1,v_t2 from t1;
   raise info 'id :=  %,name := %', v_t1.id ,v_t1.name;
end;
/


--ok
declare
  v_t1 t1%rowtype;
begin
  select id,name into v_t1 from t1;
   raise info 'id :=  %,name := %', v_t1.id ,v_t1.name;
end;
/


--failed
declare
  v_t1 t1%rowtype;
begin
  select * into v_t1 from t1;
  raise info 'age :=  %',v_t1.age;
end;
/



--age failed
declare
	v_t1 t1%rowtype;
begin
	select * into v_t1 from t1 where id=1;
	raise notice 'v_t1.id: %', v_t1.id;
	raise notice ' v_t1.name: %', v_t1.name;
	raise notice 'v_t1.age: %', v_t1.age;
end;
/


--another table
create table t2(id integer,name varchar2(256));

insert into t2 values(1,'sfjsjf');

--ok
declare
  var1 t2%rowtype;
begin
  select * into var1 from t1;
  raise info 'id :=  %,name := %', var1.id ,var1.name;
end;
/


--ok
declare
  var1 t1%rowtype;
begin
  select * into var1 from t2;
  raise info 'id :=  %,name := %', var1.id ,var1.name;
end;
/



--ok
declare
  var1 t1%rowtype;
  var2 t2%rowtype;
begin
  var1.id := 23;
  var1.name := 'xiexie';
  var2 := var1;
  var2.id := var2.id + 1;
  var2.name := var1.name;
  var1 := var2;
  raise info 'id :=  %,name := %', var1.id ,var1.name;
  raise info 'id :=  %,name := %', var2.id ,var2.name;
end;
/


drop table t1;
drop table t2;




create table t1 (id int,  age int invisible, name varchar(100));

insert into t1 (id, name, age) values (1, 'xiexie', 23);


--ok
declare
  v_t1 t1%rowtype;
begin
  select * into v_t1 from t1;
  raise info 'id :=  %,name := %', v_t1.id ,v_t1.name;
end;
/


--failed
declare
  v_t1 t1%rowtype;
begin
  select id,name,age into v_t1 from t1;
   raise info 'id :=  %,name := %', v_t1.id ,v_t1.name;
end;
/


--failed
declare
  v_t1 t1%rowtype;
  v_t2 integer;
begin
  select id,name,age into v_t1,v_t2 from t1;
   raise info 'id :=  %,name := %', v_t1.id ,v_t1.name;
end;
/


--ok
declare
  v_t1 t1%rowtype;
begin
  select id,name into v_t1 from t1;
   raise info 'id :=  %,name := %', v_t1.id ,v_t1.name;
end;
/


--failed
declare
  v_t1 t1%rowtype;
begin
  select * into v_t1 from t1;
  raise info 'age :=  %',v_t1.age;
end;
/



--age failed
declare
	v_t1 t1%rowtype;
begin
	select * into v_t1 from t1 where id=1;
	raise notice 'v_t1.id: %', v_t1.id;
	raise notice ' v_t1.name: %', v_t1.name;
	raise notice 'v_t1.age: %', v_t1.age;
end;
/


--another table
create table t2(id integer,name varchar2(256));

insert into t2 values(1,'sfjsjf');

--ok
declare
  var1 t2%rowtype;
begin
  select * into var1 from t1;
  raise info 'id :=  %,name := %', var1.id ,var1.name;
end;
/


--ok
declare
  var1 t1%rowtype;
begin
  select * into var1 from t2;
  raise info 'id :=  %,name := %', var1.id ,var1.name;
end;
/



--ok
declare
  var1 t1%rowtype;
  var2 t2%rowtype;
begin
  var1.id := 23;
  var1.name := 'xiexie';
  var2 := var1;
  var2.id := var2.id + 1;
  var2.name := var1.name;
  var1 := var2;
  raise info 'id :=  %,name := %', var1.id ,var1.name;
  raise info 'id :=  %,name := %', var2.id ,var2.name;
end;
/


drop table t1;
drop table t2;

create table t1 (id int,  age int invisible, name varchar(100));

insert into t1 (id, name, age) values (1, 'sfjsf', 23);



create table t2 (id int, name varchar(100), age int invisible);

insert into t2 (id, name, age) values (2, 'welcome to', 24);

--ok
declare
  var1 t2%rowtype;
begin
  select * into var1 from t1;
  raise info 'id :=  %,name := %', var1.id ,var1.name;
end;
/


--ok
declare
  var1 t1%rowtype;
begin
  select * into var1 from t2;
  raise info 'id :=  %,name := %', var1.id ,var1.name;
end;
/


--ok
declare
  var1 t1%rowtype;
  var2 t2%rowtype;
begin
  var1.id := 23;
  var1.name := 'xiexie';
  var2 := var1;
  var2.id := var2.id + 1;
  var2.name := var1.name;
  var1 := var2;
  raise info 'id :=  %,name := %', var1.id ,var1.name;
  raise info 'id :=  %,name := %', var2.id ,var2.name;
end;
/


--ok
declare
  cursor c1 is select * from t1;
  cursor c2 is select * from t2;
  var1 c1%rowtype;
  var2 c2%rowtype;
begin
  open c1;
  open c2;
  fetch c1 into var1;
  fetch c2 into var2;
  raise info 'id :=  %,name := %', var1.id ,var1.name;
  raise info 'id :=  %,name := %', var2.id ,var2.name;
  close c1;
  close c2;
  open c1;
  open c2;
  fetch c1 into var2;
  fetch c2 into var1;
  raise info 'id :=  %,name := %', var1.id ,var1.name;
  raise info 'id :=  %,name := %', var2.id ,var2.name;
  close c1;
  close c2;
end;
/


--ok
declare
  var1 t1%rowtype;
begin
  select * into var1 from t1;
  insert into t1 values var1;
end;
/

--2 rows
select id,age from t1 order by id,age;


delete from t1;


--ok
declare
  var1 t1%rowtype;
begin
  select * into var1 from t2;
  insert into t1 values var1;
end;
/

--1 row
select id,name,age from t1;

drop table t1;
drop table t2;


create table t1 (id int,  age varchar2(256) invisible, name varchar(100));

insert into t1 (id, name, age) values (1, 'sfjsf', 'welcome to beijing');



create table t2 (id int, name varchar(100), age varchar2(256) invisible);

insert into t2 (id, name, age) values (2, 'welcome to', 'ok weclome to');

--ok
declare
  var1 t2%rowtype;
begin
  select * into var1 from t1;
  raise info 'id :=  %,name := %', var1.id ,var1.name;
end;
/


--ok
declare
  var1 t1%rowtype;
begin
  select * into var1 from t2;
  raise info 'id :=  %,name := %', var1.id ,var1.name;
end;
/


--ok
declare
  var1 t1%rowtype;
  var2 t2%rowtype;
begin
  var1.id := 23;
  var1.name := 'xiexie';
  var2 := var1;
  var2.id := var2.id + 1;
  var2.name := var1.name;
  var1 := var2;
  raise info 'id :=  %,name := %', var1.id ,var1.name;
  raise info 'id :=  %,name := %', var2.id ,var2.name;
end;
/


--ok
declare
  cursor c1 is select * from t1;
  cursor c2 is select * from t2;
  var1 c1%rowtype;
  var2 c2%rowtype;
begin
  open c1;
  open c2;
  fetch c1 into var1;
  fetch c2 into var2;
  raise info 'id :=  %,name := %', var1.id ,var1.name;
  raise info 'id :=  %,name := %', var2.id ,var2.name;
  close c1;
  close c2;
  open c1;
  open c2;
  fetch c1 into var2;
  fetch c2 into var1;
  raise info 'id :=  %,name := %', var1.id ,var1.name;
  raise info 'id :=  %,name := %', var2.id ,var2.name;
  close c1;
  close c2;
end;
/


--ok
declare
  var1 t1%rowtype;
begin
  select * into var1 from t1;
  insert into t1 values var1;
end;
/

--2 rows
select id,age from t1 order by id,age;


delete from t1;


--ok
declare
  var1 t1%rowtype;
begin
  select * into var1 from t2;
  insert into t1 values var1;
end;
/

--1 row
select id,name,age from t1;

drop table t1;
drop table t2;


---test droppped COLUMNS
create table t1 (id2 integer,id int,  age varchar2(256) invisible, name varchar(100));

insert into t1 (id2,id, name, age) values (1,1, 'sfjsf', 'welcome to beijing');


create table t2 (id int, name varchar(100), age varchar2(256) invisible, name1 varchar2(256));

insert into t2 (id, name, age,name1) values (2, 'welcome to', 'ok weclome to', 'testdrop');

--drop column
alter table t1 drop column id2;
alter table t2 drop column name1;

--ok
declare
  var1 t2%rowtype;
begin
  select * into var1 from t1;
  raise info 'id :=  %,name := %', var1.id ,var1.name;
end;
/


--ok
declare
  var1 t1%rowtype;
begin
  select * into var1 from t2;
  raise info 'id :=  %,name := %', var1.id ,var1.name;
end;
/


--ok
declare
  var1 t1%rowtype;
  var2 t2%rowtype;
begin
  var1.id := 23;
  var1.name := 'xiexie';
  var2 := var1;
  var2.id := var2.id + 1;
  var2.name := var1.name;
  var1 := var2;
  raise info 'id :=  %,name := %', var1.id ,var1.name;
  raise info 'id :=  %,name := %', var2.id ,var2.name;
end;
/


--ok
declare
  cursor c1 is select * from t1;
  cursor c2 is select * from t2;
  var1 c1%rowtype;
  var2 c2%rowtype;
begin
  open c1;
  open c2;
  fetch c1 into var1;
  fetch c2 into var2;
  raise info 'id :=  %,name := %', var1.id ,var1.name;
  raise info 'id :=  %,name := %', var2.id ,var2.name;
  close c1;
  close c2;
  open c1;
  open c2;
  fetch c1 into var2;
  fetch c2 into var1;
  raise info 'id :=  %,name := %', var1.id ,var1.name;
  raise info 'id :=  %,name := %', var2.id ,var2.name;
  close c1;
  close c2;
end;
/


--ok
declare
  var1 t1%rowtype;
begin
  select * into var1 from t1;
  insert into t1 values var1;
end;
/

--2 rows
select id,age from t1 order by id,age;


delete from t1;


--ok
declare
  var1 t1%rowtype;
begin
  select * into var1 from t2;
  insert into t1 values var1;
end;
/

--1 row
select id,name,age from t1;

drop table t1;
drop table t2;

--bug#Z1392
\set SEND_SPACE_ONE_COMMENTS true

create or replace function f_re11(id integer) return integer IS
/* this is a test comment */
BEGIN
  return id;
end;
/

\sf f_re11

select prosrc from pg_proc where proname='f_re11';

create or replace procedure p_rel1(id integer) as
/* this is a test comment */
BEGIN
 raise info 'p_rel1';
end;
/

\sf p_rel1

select prosrc from pg_proc where proname='p_rel1';

create or replace package test_pkg1 is /* a test package */
 var1 integer;
 /* a test */
 function test_f(id integer) return integer;
end;
/

create or replace package body test_pkg1 IS
  /* a test package body */
 function test_f(id integer) return integer IS
 BEGIN
  return 1;
 end;
BEGIN
   /* a test */
   var1 := 1;
end;
/

select pkgsrc,bodysrc from pg_package left join pg_package_body on pg_package.oid = pg_package_body.pkgoid where pkgname='test_pkg1';

select f_re11(23) from dual;
call p_rel1(23);
select test_pkg1.test_f(23) from dual;

drop function f_re11;
drop procedure p_rel1;
drop package test_pkg1;

---save the newline after a newline
create or replace function f_re11(id integer) return integer IS

/* this is a test comment */
BEGIN
  return id;
end;
/

\sf f_re11

select prosrc from pg_proc where proname='f_re11';


create or replace procedure p_rel1(id integer) as /* a test */
/* this is a test comment */

BEGIN

 raise info 'p_rel1';
end;
/

\sf p_rel1

select prosrc from pg_proc where proname='p_rel1';


create or replace package test_pkg1 is /* a test package */

 var1 integer;
 /* a test */
 function test_f(id integer) return integer;
end;
/

create or replace package body test_pkg1 IS
  /* a test package body */
  
 function test_f(id integer) return integer IS
 BEGIN
  return 1;
 end;
BEGIN
   /* a test */
   var1 := 1;
end;
/

select pkgsrc,bodysrc from pg_package left join pg_package_body on pg_package.oid = pg_package_body.pkgoid where pkgname='test_pkg1';

select f_re11(23) from dual;
call p_rel1(23);
select test_pkg1.test_f(23) from dual;

--clean data
drop function f_re11;
drop procedure p_rel1;
drop package test_pkg1;

/* Begin - OA-REQ-2454 */

CREATE TABLE log_table_2454 (
    log_id      NUMBER PRIMARY KEY,
    log_date    DATE,
    log_message VARCHAR2(4000),
    created_by  VARCHAR2(100)
);

CREATE SEQUENCE log_seq_2454 START WITH 1 INCREMENT BY 1;

CREATE OR REPLACE PACKAGE test_pkg_2454 IS
    PROCEDURE log(p_message VARCHAR2);

    PROCEDURE run_demo(p_param VARCHAR2);
END test_pkg_2454;
/

CREATE OR REPLACE PACKAGE BODY test_pkg_2454 IS
    PROCEDURE log(p_message VARCHAR2) IS
        PRAGMA AUTONOMOUS_TRANSACTION;
    BEGIN
        INSERT INTO log_table_2454 (log_id, log_date, log_message, created_by)
        VALUES (log_seq_2454.NEXTVAL, to_date('2026-06-09','yyyy-mm-dd'), p_message, 'Highgo');
        COMMIT;
    END log;

    PROCEDURE run_demo(p_param VARCHAR2) IS
    BEGIN
        log('run_demo Start, Parameter = ' || p_param);
        DBMS_OUTPUT.PUT_LINE('Exec run_demo, Parameter = ' || p_param);
        log('run_demo Done, Parameter = ' || p_param);
        COMMIT;
    END run_demo;

END test_pkg_2454;
/


SET SERVEROUTPUT ON;

BEGIN
    test_pkg_2454.run_demo('Test 1');
    test_pkg_2454.run_demo('Test 2');
END;
/

SELECT * FROM log_table_2454 ORDER BY log_id;


truncate table log_table_2454;
DROP PACKAGE test_pkg_2454;


CREATE OR REPLACE PACKAGE test_pkg_2454
IS
    PROCEDURE log;
    PROCEDURE run_demo(p_param VARCHAR2);
END test_pkg_2454;
/

CREATE OR REPLACE PACKAGE BODY test_pkg_2454
IS
    PROCEDURE log IS
        PRAGMA AUTONOMOUS_TRANSACTION;
    BEGIN
        INSERT INTO log_table_2454 (
            log_id,
            log_date,
            log_message,
            created_by
        )
        VALUES (
            log_seq_2454.NEXTVAL,
            to_date('2026-06-10','yyyy-mm-dd'),
            'Parameterless log called',
            'Highgo'
        );

        COMMIT;
    END log;


    PROCEDURE run_demo(p_param VARCHAR2) IS
    BEGIN
        log;

        DBMS_OUTPUT.PUT_LINE('Exec run_demo, Parameter = ' || p_param);

        log;

        COMMIT;
    END run_demo;

END test_pkg_2454;
/


SET SERVEROUTPUT ON;

BEGIN
    test_pkg_2454.log;
    test_pkg_2454.run_demo('Test 3');
    test_pkg_2454.run_demo('Test 4');
END;
/


CALL test_pkg_2454.log();


SELECT log_id,
       TO_CHAR(log_date, 'YYYY-MM-DD HH24:MI:SS') AS log_date,
       log_message,
       created_by
FROM log_table_2454
ORDER BY log_id;


--cleanup
DROP PACKAGE test_pkg_2454;
DROP SEQUENCE log_seq_2454;
DROP TABLE log_table_2454;
SET SERVEROUTPUT OFF;
/* End - OA-REQ-2454 */
