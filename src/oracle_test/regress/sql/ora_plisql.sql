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

-- error, need convert int to number(38) within anonymous.
DECLARE
 b int;
BEGIN
  call test_p(3, b);
  RAISE NOTICE 'b: %', b;
END;
/

-- error, need convert int to number(38) within anonymous.
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
