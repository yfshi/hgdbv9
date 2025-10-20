----view
----function
----procedure
----trigger
----tablespace
----index
----constraint
----ref constraint
----table
----global temporary table 
----partition table
----identity column
----invisble column
----virtual column
----package, package body
----dblink
----sequence
CREATE USER system WITH superuser;

\c - system

SET ivorysql.identifier_case_switch = interchange;

--
-- view
--
CREATE TABLE t_503 (i int);
CREATE VIEW v_t_503 AS SELECT * FROM t_503;
SELECT DBMS_METADATA.get_ddl('VIEW', 'V_T_503', 'PUBLIC') from dual;
DROP VIEW v_t_503;
DROP TABLE t_503;

--view name with quotes
CREATE TABLE "t_503" (i int);
CREATE VIEW "v_t_503" AS SELECT * FROM "t_503";
SELECT DBMS_METADATA.get_ddl('VIEW', 'v_t_503', 'PUBLIC') from dual;
DROP VIEW "v_t_503";
DROP TABLE "t_503";

CREATE TABLE "TV" (i int);
CREATE VIEW "V1" AS SELECT * FROM "TV";
SELECT DBMS_METADATA.get_ddl('VIEW', 'V1', 'PUBLIC') from dual;
DROP VIEW "V1";
DROP TABLE "TV";

CREATE TABLE "tv2" (i int);
CREATE VIEW "v2" AS SELECT * FROM "tv2";
SELECT DBMS_METADATA.get_ddl('VIEW', 'v2', 'PUBLIC') from dual;
DROP VIEW "v2";
DROP TABLE "tv2";


--MATERIALIZED VIEW
CREATE TABLE tv3 (i int);
CREATE MATERIALIZED VIEW mivt AS SELECT * FROM tv3 ;
SELECT DBMS_METADATA.get_ddl('VIEW', 'MIVT', 'PUBLIC') from dual;
DROP MATERIALIZED VIEW mivt;

CREATE MATERIALIZED VIEW mivt2 AS SELECT * FROM tv3 LIMIT 3;
SELECT DBMS_METADATA.get_ddl('VIEW', 'MIVT2', 'PUBLIC') from dual;
DROP MATERIALIZED VIEW mivt2;
DROP TABLE tv3;

--FORCE VIEW
CREATE FORCE VIEW fv1 AS SELECT * FROM tv4;
SELECT DBMS_METADATA.get_ddl('VIEW', 'FV1', 'PUBLIC') from dual;
DROP VIEW fv1;

CREATE TABLE "tv4" ("i" int);
CREATE FORCE VIEW "fv2" AS SELECT * FROM "tv4";
SELECT DBMS_METADATA.get_ddl('VIEW', 'fv2', 'PUBLIC') from dual;
DROP VIEW "fv2";
DROP TABLE "tv4";

--
-- function
--
CREATE OR REPLACE function f_metadata(a IN int) RETURN int
IS
  i int := a;
BEGIN
 RETURN i;
END;
/
select dbms_metadata.get_ddl('FUNCTION', 'F_METADATA', 'PUBLIC') from dual;
DROP FUNCTION f_metadata(int);

--function name with quotes
CREATE OR REPLACE function "f_metadata"(a IN int) RETURN int
IS
  i int := a;
BEGIN
 RETURN i;
END;
/
SELECT sys.dbms_metadata.get_ddl('FUNCTION', 'f_metadata', 'PUBLIC') from dual;
DROP FUNCTION "f_metadata"(int);


CREATE SCHEMA "PLPM";
CREATE OR REPLACE FUNCTION "PLPM"."BP_A_CRMONTH"(IN i_branid varchar2, IN i_date varchar2, OUT o_code int, OUT o_msg varchar2)
RETURN void AS
  name varchar2(100);
BEGIN
  o_code := 100;
  o_msg := 'fun1';
  name := PLPM.FN_GETPNAME;
  dbms_output.put_line('in frist function, name = ' || name);
END;
/

select dbms_metadata.get_ddl('FUNCTION', 'BP_A_CRMONTH','PLPM');
DROP FUNCTION "PLPM"."BP_A_CRMONTH"(IN i_branid varchar2, IN i_date varchar2, OUT o_code int, OUT o_msg varchar2);


--
-- procedure
--
CREATE OR REPLACE PROCEDURE p_metadata(a IN int) AS
  i int := a;
BEGIN
  raise notice '%', i;
END;
/
select dbms_metadata.get_ddl('PROCEDURE', 'P_METADATA', 'PUBLIC') from dual;
DROP PROCEDURE p_metadata(int);

--procedure name with quotes
CREATE OR REPLACE PROCEDURE "p_metadata"(a IN int) AS
  i int := a;
BEGIN
  raise notice '%', i;
END;
/
select dbms_metadata.get_ddl('PROCEDURE', 'p_metadata', 'PUBLIC') from dual;
DROP PROCEDURE "p_metadata"(int);

CREATE OR REPLACE PROCEDURE "PLPM"."BP_A_CRMONTH"(IN i_branid varchar2, IN i_date varchar2, OUT o_code varchar2, OUT o_msg varchar2)
AS
 name varchar2(100);
BEGIN
  o_code := 'p';
  o_msg := 'pro2';
  name := PLPM.FN_GETPNAME();
  dbms_output.put_line('in procedure, name = ' || name);
END;
/

select dbms_metadata.get_ddl('PROCEDURE', 'BP_A_CRMONTH','PLPM');
DROP PROCEDURE "PLPM"."BP_A_CRMONTH"(IN i_branid varchar2, IN i_date varchar2, OUT o_code varchar2, OUT o_msg varchar2);
DROP SCHEMA "PLPM";


--
-- trigger
--
CREATE TABLE t_trigger(department_id integer,salary integer, name varchar(23));
CREATE OR REPLACE FUNCTION dummy_update_func() RETURNS trigger AS $$
BEGIN
  RAISE NOTICE 'dummy_update_func(%) called: action = %, old = %, new = %',
    TG_ARGV[0], TG_OP, OLD, NEW;
  RETURN NEW;
END;
$$ LANGUAGE plisql;
/

CREATE OR REPLACE TRIGGER ttserial
	BEFORE INSERT OR UPDATE ON t_trigger
	FOR EACH ROW
	EXECUTE FUNCTION
	public.dummy_update_func();
/

select dbms_metadata.get_ddl('TRIGGER', 'TTSERIAL','PUBLIC') from dual;

DROP TRIGGER ttserial ON t_trigger;
DROP FUNCTION dummy_update_func();
DROP TABLE t_trigger;


--tablespace
--select dbms_metadata.get_ddl('TABLESPACE', 'PG_DEFAULT') from dual;


--
-- index
--
CREATE TABLE t_503(i int);
CREATE INDEX idx_503 on t_503(i);
select dbms_metadata.get_ddl('INDEX', 'IDX_503', 'PUBLIC') from dual;

DROP INDEX idx_503;

--index name with quotes
CREATE INDEX "IDX_503" on t_503(i);
select dbms_metadata.get_ddl('INDEX', 'IDX_503', 'PUBLIC') from dual;
DROP INDEX "IDX_503";

CREATE INDEX "idx_503" on t_503(i);
select dbms_metadata.get_ddl('INDEX', 'idx_503', 'PUBLIC') from dual;

DROP INDEX "idx_503";
DROP TABLE t_503;

CREATE SCHEMA "s2";
set search_path="s2";

CREATE TABLE "s2".t_503(i int);
CREATE INDEX "idx_503" on "s2".t_503(i);
select dbms_metadata.get_ddl('INDEX', 'idx_503', 's2') from dual;

DROP INDEX "s2"."idx_503";
DROP TABLE "s2".t_503;
RESET search_path;
DROP SCHEMA "s2";

--
-- constraint
--
CREATE TABLE t_503(i int  CONSTRAINT firstkey PRIMARY KEY);
select dbms_metadata.get_ddl('CONSTRAINT', 'FIRSTKEY', 'PUBLIC') from dual;
DROP TABLE t_503;

--constraint name with quotes
CREATE TABLE t_503("i" int  CONSTRAINT "FIRSTKEY" PRIMARY KEY);
select dbms_metadata.get_ddl('CONSTRAINT', 'FIRSTKEY', 'PUBLIC') from dual;
DROP TABLE t_503;


CREATE TABLE t_503("I" int  CONSTRAINT "firstkey" PRIMARY KEY);
select dbms_metadata.get_ddl('CONSTRAINT', 'firstkey', 'PUBLIC') from dual;
DROP TABLE t_503;

--ref constraint
CREATE TABLE Persons(Id_P int PRIMARY KEY);
CREATE TABLE Orders(Id_O int NOT NULL PRIMARY KEY, OrderNo int NOT NULL, Id_P int REFERENCES  Persons);
ALTER TABLE Orders ADD CONSTRAINT order_f_key foreign key (Id_P) references Persons (Id_P);
select dbms_metadata.get_ddl('REF_CONSTRAINT', 'ORDER_F_KEY', 'PUBLIC') from dual;
DROP TABLE Orders cascade;
DROP TABLE Persons cascade;

--ref constraint name with quotes
CREATE TABLE Persons("id_p" int PRIMARY KEY);
CREATE TABLE Orders(Id_O int NOT NULL PRIMARY KEY, OrderNo int NOT NULL, "id_p" int REFERENCES  Persons);
ALTER TABLE Orders ADD CONSTRAINT "order_f_key" foreign key ("id_p") references Persons ("id_p");
select dbms_metadata.get_ddl('REF_CONSTRAINT', 'order_f_key', 'PUBLIC') from dual;
DROP TABLE Orders cascade;
DROP TABLE Persons cascade;

CREATE TABLE Persons("ID_P" int PRIMARY KEY);
CREATE TABLE Orders(Id_O int NOT NULL PRIMARY KEY, OrderNo int NOT NULL, "ID_P" int REFERENCES  Persons);
ALTER TABLE Orders ADD CONSTRAINT "ORDER_F_KEY" foreign key ("ID_P") references Persons ("ID_P");
select dbms_metadata.get_ddl('REF_CONSTRAINT', 'ORDER_F_KEY', 'PUBLIC') from dual;
DROP TABLE Orders cascade;
DROP TABLE Persons cascade;


--table
CREATE TABLE t_503(i int  CONSTRAINT firstkey PRIMARY KEY, j varchar(20) default 'The DBMS_METADATA package provides a way for you to retrieve metadata from the database dictionary as XML or creation DDL and to submit the XML to re-create the object');
select dbms_metadata.get_ddl('TABLE', 'T_503', 'PUBLIC') from dual;
DROP TABLE t_503;



--
-- table name with quotes
--
create table "a_503"(id1 int);
create table "A_503"(id2 int);
create table "A#A_503"(id3 int);
create table "a#a_503"(id4 int);

select dbms_metadata.get_ddl('TABLE', 'a_503', 'PUBLIC') from dual;
select dbms_metadata.get_ddl('TABLE', 'A_503', 'PUBLIC') from dual;
select dbms_metadata.get_ddl('TABLE', 'A#A_503', 'PUBLIC') from dual;
select dbms_metadata.get_ddl('TABLE', 'a#a_503', 'PUBLIC') from dual;

drop table "a_503";
drop table "A_503";
drop table "A#A_503";
drop table "a#a_503";


create table tab_618_constraint(id int primary key,a int unique,b int not null, c int check (c between 1 and 100));
create table tab_618_constraint_2(id int, a int, b int, c int,
constraint con_meta_for foreign key(a) references tab_618_constraint(id) ,
constraint con_meta_check check (c between 1 and 100),
constraint con_meta_unique unique(b)
);
select DBMS_METADATA.GET_DDL('TABLE','TAB_618_CONSTRAINT_2','PUBLIC') from dual;
drop table tab_618_constraint_2;
drop table tab_618_constraint;


create table tab_constraint(id int primary key,a int not null, b int check (b between 1 and 100));
select DBMS_METADATA.GET_DDL('TABLE','TAB_CONSTRAINT', 'PUBLIC') from dual;
drop table tab_constraint;


create schema user_meta;
set search_path=user_meta;
create table user_meta.t_739(id int);
select DBMS_METADATA.GET_DDL('TABLE','T_739', 'USER_META');
drop table user_meta.t_739;
reset search_path;
drop schema user_meta cascade;


--do not specify schema
CREATE TABLE t1(id int);
SELECT dbms_metadata.get_ddl('TABLE', 'T1') FROM DUAL;
DROP TABLE t1;


CREATE TABLE citys(id number(6) PRIMARY KEY, name varchar2(64 char));
CREATE TABLE employees_demo(
   employee_id 	NUMBER(6) PRIMARY KEY,
   city_id    NUMBER(6),
   first_name VARCHAR2(20),
   last_name  VARCHAR2(25) CONSTRAINT emp_ln_nn_demo NOT NULL,
   email      VARCHAR2(64) NOT NULL,
   hire_date  DATE NOT NULL,
   salary     NUMBER(8,2) NOT NULL,
   dn         VARCHAR2(300),
   CONSTRAINT emp_salary_min CHECK (salary > 0),
   CONSTRAINT emp_email_uk UNIQUE (email),
   CONSTRAINT city_id_fk FOREIGN KEY (city_id) REFERENCES citys(id)
);

set plsql.default_into_strict=on;
SELECT dbms_metadata.get_ddl('TABLE', 'EMPLOYEES_DEMO') FROM dual;
set plsql.default_into_strict=off;
SELECT dbms_metadata.get_ddl('TABLE', 'EMPLOYEES_DEMO') FROM dual;
DROP TABLE employees_demo;
DROP TABLE citys;


--
-- GLOBAL TEMPORARY TABLE
--
CREATE GLOBAL TEMPORARY TABLE test_gl_trans(id integer,name varchar2(23));
CREATE GLOBAL TEMPORARY TABLE test_gl_session(id integer,name varchar2(23)) ON COMMIT PRESERVE ROWS;

SELECT dbms_metadata.get_ddl('TABLE', 'TEST_GL_TRANS') FROM DUAL;
SELECT dbms_metadata.get_ddl('TABLE', 'TEST_GL_SESSION') FROM DUAL;

DROP TABLE test_gl_trans;
DROP TABLE test_gl_session;


--
-- partition table
--
CREATE TABLE prt (a int, b int, c varchar(4000)) PARTITION BY RANGE(a);
CREATE TABLE prt_p1 PARTITION OF prt FOR VALUES FROM (0) TO (250);
CREATE TABLE prt_p2 PARTITION OF prt FOR VALUES FROM (500) TO (600);
CREATE INDEX iprt_p1_a on prt_p1(a);
SELECT dbms_metadata.get_ddl('TABLE','PRT', 'PUBLIC') from dual;
DROP TABLE prt;

CREATE TABLE plt1_e (a int, b int, c varchar(4000)) PARTITION BY LIST(ltrim(c, 'A'));
CREATE TABLE plt1_e_p1 PARTITION OF plt1_e FOR VALUES IN ('0000', '0003', '0004', '0010');
SELECT dbms_metadata.get_ddl('TABLE','PLT1_E', 'PUBLIC') from dual;
DROP TABLE plt1_e;

CREATE TABLE pht1 (a int, b int, c varchar(4000)) PARTITION BY HASH(c);
CREATE TABLE pht1_p1 PARTITION OF pht1 FOR VALUES WITH (MODULUS 3, REMAINDER 0);
SELECT dbms_metadata.get_ddl('TABLE','PHT1', 'PUBLIC') from dual;
DROP TABLE pht1;


--
-- identity column
--
CREATE TABLE t_default (
	id NUMBER GENERATED BY DEFAULT AS IDENTITY,
	description text
);
SELECT dbms_metadata.get_ddl('TABLE','T_DEFAULT', 'PUBLIC') from dual;
DROP TABLE t_default;


--BY DEFAULT ON NULL
CREATE TABLE t_default_null (
	id NUMBER GENERATED BY DEFAULT ON NULL AS IDENTITY,
	description text
);
SELECT dbms_metadata.get_ddl('TABLE','T_DEFAULT_NULL', 'PUBLIC') from dual;
DROP TABLE t_default_null;


CREATE TABLE t_1(
	id		  int GENERATED BY DEFAULT AS IDENTITY INCREMENT by 2,
	description text
);
SELECT dbms_metadata.get_ddl('TABLE','T_1', 'PUBLIC') from dual;
DROP TABLE t_1;

CREATE TABLE t_2(userid number generated always as identity maxvalue -5 minvalue -10 start with 7  increment by -2, uname varchar2(20));
SELECT dbms_metadata.get_ddl('TABLE','T_2', 'PUBLIC') from dual;
DROP TABLE t_2;

CREATE TABLE t_3(userid number generated BY DEFAULT as identity maxvalue -5 minvalue -10 start with -7  increment by -2, uname varchar2(20));
SELECT dbms_metadata.get_ddl('TABLE','T_3', 'PUBLIC') from dual;
DROP TABLE t_3;

CREATE TABLE t_4(userid number generated BY DEFAULT ON NULL as identity maxvalue -5 minvalue -10 start with -7  increment by -2, uname varchar2(20));
SELECT dbms_metadata.get_ddl('TABLE','T_4', 'PUBLIC') from dual;
DROP TABLE t_4;

--
-- INVISIBLE column
--
CREATE TABLE tab (id NUMBER,description VARCHAR2(50) INVISIBLE);
SELECT dbms_metadata.get_ddl('TABLE','TAB', 'PUBLIC') from dual;
DROP TABLE tab;

CREATE TABLE tab2 (id NUMBER);
ALTER TABLE tab2 ADD age INT INVISIBLE;
SELECT dbms_metadata.get_ddl('TABLE','TAB2', 'PUBLIC') from dual;
DROP TABLE tab2;

--
-- Virtual column
--
CREATE TABLE vtest1 (a int PRIMARY KEY, b int GENERATED ALWAYS AS (a * 2));
SELECT dbms_metadata.get_ddl('TABLE','VTEST1', 'PUBLIC') from dual;
DROP TABLE vtest1;

CREATE TABLE vtest_idx (a int, b int GENERATED ALWAYS AS (a * 2) PRIMARY KEY);
SELECT dbms_metadata.get_ddl('TABLE','VTEST_IDX', 'PUBLIC') from dual;
DROP TABLE vtest_idx;

CREATE TABLE vtest2 (a int PRIMARY KEY, b int GENERATED ALWAYS AS (a * 2) check (b > 5));
SELECT dbms_metadata.get_ddl('TABLE','VTEST2', 'PUBLIC') from dual;
DROP TABLE vtest2;

CREATE TABLE vtest3 (id number,product varchar2(50),price number(10,2),price_with_tax int generated always as (round(price*1.2,2)) virtual);
SELECT dbms_metadata.get_ddl('TABLE','VTEST3', 'PUBLIC') from dual;
DROP TABLE vtest3;

CREATE OR REPLACE FUNCTION my_func(a int) RETURN INT deterministic
AS
BEGIN
  RETURN a + 5;
END;
/
CREATE TABLE vtest4 (a int PRIMARY KEY, b int GENERATED ALWAYS AS (my_func(a)));
SELECT dbms_metadata.get_ddl('TABLE','VTEST4', 'PUBLIC') from dual;
DROP TABLE vtest4;
DROP FUNCTION my_func;


--
-- package, package body
--
create or replace package pkg_503 as
  i int;
  function test(id integer) return integer;
end;
/

create or replace package body pkg_503 as
 function test(id integer) return integer as
 var1 integer;
 begin
    var1 := id;
    return var1 + 1;
  end;
end;
/

select dbms_metadata.get_ddl('PACKAGE', 'PKG_503', 'PUBLIC') from dual;
select dbms_metadata.get_ddl('PACKAGE_BODY', 'PKG_503', 'PUBLIC') from dual;
drop package pkg_503;

--
-- package, package body name with quotes
--
create or replace package "pkg_503" as
  i int;
  function test(id integer) return integer;
end "pkg_503";
/

create or replace package body "pkg_503" as
 function test(id integer) return integer is
 var1 integer;
 begin
    var1 := id;
    return var1 + 1;
  end;
end "pkg_503";
/

select dbms_metadata.get_ddl('PACKAGE', 'pkg_503', 'PUBLIC') from dual;
select dbms_metadata.get_ddl('PACKAGE_BODY', 'pkg_503', 'PUBLIC') from dual;
drop package "pkg_503";

create or replace package "PKG_503" as
  i int;
  function test(id integer) return integer;
end "PKG_503";
/

create or replace package body "PKG_503" as
 function test(id integer) return integer is
 var1 integer;
 begin
    var1 := id;
    return var1 + 1;
  end;
end "PKG_503";
/

select dbms_metadata.get_ddl('PACKAGE', 'PKG_503', 'PUBLIC') from dual;
select dbms_metadata.get_ddl('PACKAGE_BODY', 'PKG_503', 'PUBLIC') from dual;
drop package "PKG_503";


CREATE OR REPLACE PACKAGE pkg_meta AS
  PROCEDURE print_numlist (nums INT);
  END pkg_meta;
/
CREATE OR REPLACE PACKAGE BODY pkg_meta AS
  PROCEDURE print_numlist (nums INT) IS
  BEGIN
    NULL;
  END print_numlist;
END pkg_meta;
/
select DBMS_METADATA.GET_DDL('PACKAGE','PKG_META', 'PUBLIC') from dual;
drop package pkg_meta;

--
-- DBLINK
--
create database link dblnk1
connect to system identified by "oracle"
using '(DESCRIPTION=(ADDRESS_LIST=(ADDRESS=(PROTOCOL=TCP)(HOST=127.0.0.1)(PORT=5432)))
(CONNECT_DATA=(SERVICE_NAME=postgres)))';
SELECT DBMS_METADATA.GET_DDL('DB_LINK','DBLNK1', 'PUBLIC') from dual;
DROP database link dblnk1;

-- Shared DBLINK
DO $d$
    BEGIN
        EXECUTE $$ create shared public database link "dblnk2"
		connect to system123 identified by ivy
		AUTHENTICATED BY u123 IDENTIFIED BY "ivy123"
        	using '(DESCRIPTION =
            (ADDRESS = (PROTOCOL = TCP)(PORT = 5432))
            (CONNECT_DATA =
                (SERVER = DEDICATED)
                (SERVICE_NAME = tdblnk)
            ))'$$;
    END;
$d$;

SELECT DBMS_METADATA.GET_DDL('DB_LINK','dblnk2', 'PUBLIC') from dual;
DROP public database link "dblnk2";

-- public DBLINK
create public database link "DBLNK3"
connect to "u1" identified by "ORA"
using '(DESCRIPTION=(ADDRESS_LIST=(ADDRESS=(PROTOCOL=TCP)(HOST=127.0.0.1)(PORT=5432)))
(CONNECT_DATA=(SERVICE_NAME=postgres)))';
SELECT DBMS_METADATA.GET_DDL('DB_LINK','DBLNK3', 'PUBLIC') from dual;
DROP public database link "DBLNK3";

-- CURRENT_USER
DO $d$
    BEGIN
        EXECUTE $$ create public database link dblnk4
        	connect to CURRENT_USER
        	using '(DESCRIPTION =
            (ADDRESS = (PROTOCOL = TCP)(PORT = 5432))
            (CONNECT_DATA =
                (SERVER = DEDICATED)
                (SERVICE_NAME = postgres)
            ))'$$;
    END;
$d$;
SELECT DBMS_METADATA.GET_DDL('DB_LINK','DBLNK4', 'PUBLIC') from dual;
DROP public database link dblnk4;

-- Connected User
DO $d$
    BEGIN
        EXECUTE $$ create public database link dblnk5
        	using '(DESCRIPTION =
            (ADDRESS = (PROTOCOL = TCP)(PORT = 5432))
            (CONNECT_DATA =
                (SERVER = DEDICATED)
                (SERVICE_NAME = tdblnk)
            ))'$$;
    END;
$d$;
SELECT DBMS_METADATA.GET_DDL('DB_LINK','DBLNK5', 'PUBLIC') from dual;
DROP public database link dblnk5;

--
-- SEQUENCE
--
CREATE SEQUENCE seq1;
SELECT dbms_metadata.get_ddl('SEQUENCE','SEQ1', 'PUBLIC') from dual;
DROP SEQUENCE seq1;

CREATE SEQUENCE "seq2" nocache;
SELECT dbms_metadata.get_ddl('SEQUENCE','seq2', 'PUBLIC')from dual;
DROP SEQUENCE "seq2";

CREATE SEQUENCE "SEQ3" INCREMENT BY 1 START WITH 32 MINVALUE 10 MAXVALUE 50;
SELECT dbms_metadata.get_ddl('SEQUENCE','SEQ3', 'PUBLIC') from dual;
DROP SEQUENCE "SEQ3";

CREATE SEQUENCE seq4 MAXVALUE 999999999 MINVALUE 999 START WITH 1000 INCREMENT BY 1 NOCACHE CYCLE;
SELECT dbms_metadata.get_ddl('SEQUENCE','SEQ4', 'PUBLIC') from dual;
select seq4.nextval from dual;
SELECT seq4.currval from dual;
SELECT dbms_metadata.get_ddl('SEQUENCE', 'SEQ4', 'PUBLIC') from dual;
DROP SEQUENCE seq4;

CREATE SEQUENCE seq5 INCREMENT BY 2 START WITH 30 MINVALUE 10 MAXVALUE 50 CYCLE CACHE 2;
SELECT dbms_metadata.get_ddl('SEQUENCE','SEQ5', 'PUBLIC') from dual;
DROP SEQUENCE seq5;

create sequence seq_shard1 shard;
SELECT dbms_metadata.get_ddl('SEQUENCE','SEQ_SHARD1', 'PUBLIC') from dual;
DROP SEQUENCE seq_shard1;

create sequence seq_shard2 noshard;
SELECT dbms_metadata.get_ddl('SEQUENCE','SEQ_SHARD2', 'PUBLIC') from dual;
DROP SEQUENCE seq_shard2;

create sequence seq_shard3 shard extend ;
SELECT dbms_metadata.get_ddl('SEQUENCE','SEQ_SHARD3', 'PUBLIC') from dual;
DROP SEQUENCE seq_shard3;

--drop user sytsem
declare
    name text;
begin
        select into name 10::regrole;
		execute format('set session authorization %s;',name);
end;
/

DROP USER system;
