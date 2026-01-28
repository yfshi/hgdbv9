--example come from oracle internet
--ok
create table sales_input_table(product_id integer,customer_id integer,
							weekly_st date, sales_sun integer, sales_mon integer,
							sales_tue integer,sales_wed integer,sales_thu integer,
							sales_fri integer,sales_sat integer);
--ok
create table sales(prod_id integer,cust_id integer, time_id date,amount integer);

--insert some data
insert into sales_input_table values(111,222,'01-10-00', 100,200,300,400,500,600,700);
insert into sales_input_table values(222,333,'08-10-00',200,300,400,500,600,700,800);
insert into sales_input_table values(333,444,'15-10-00',300,400,500,600,700,800,900);

--insert all
--21 rows
INSERT ALL
      INTO sales (prod_id, cust_id, time_id, amount)
      VALUES (product_id, customer_id, weekly_start_date, sales_sun)
      INTO sales (prod_id, cust_id, time_id, amount)
      VALUES (product_id, customer_id, weekly_start_date+1, sales_mon)
      INTO sales (prod_id, cust_id, time_id, amount)
      VALUES (product_id, customer_id, weekly_start_date+2, sales_tue)
      INTO sales (prod_id, cust_id, time_id, amount)
      VALUES (product_id, customer_id, weekly_start_date+3, sales_wed)
      INTO sales (prod_id, cust_id, time_id, amount)
      VALUES (product_id, customer_id, weekly_start_date+4, sales_thu)
      INTO sales (prod_id, cust_id, time_id, amount)
      VALUES (product_id, customer_id, weekly_start_date+5, sales_fri)
      INTO sales (prod_id, cust_id, time_id, amount)
      VALUES (product_id, customer_id, weekly_start_date+6, sales_sat)
   SELECT product_id, customer_id, weekly_st as weekly_start_date, sales_sun,
      sales_mon, sales_tue, sales_wed, sales_thu, sales_fri, sales_sat
      FROM sales_input_table ORDER BY product_id;

--select ok
--21 rows
SELECT * FROM sales
   ORDER BY prod_id, cust_id, time_id;

--ok
CREATE TABLE small_orders
   (order_id       NUMBER(12)   NOT NULL,
    customer_id    NUMBER(6)    NOT NULL,
    order_total    NUMBER(8,2),
    sales_rep_id   NUMBER(6)
   );

CREATE TABLE medium_orders AS SELECT * FROM small_orders;
CREATE TABLE large_orders AS SELECT * FROM small_orders;
CREATE TABLE special_orders
   (order_id       NUMBER(12)    NOT NULL,
    customer_id    NUMBER(6)     NOT NULL,
    order_total    NUMBER(8,2),
    sales_rep_id   NUMBER(6),
    credit_limit   NUMBER(9,2),
    cust_email     VARCHAR2(40)
   );

create table multi_orders as select * from special_orders;
create table multi_customers(customer_id NUMBER(6), credit_limit   NUMBER(9,2),
    cust_email     VARCHAR2(40));

--insert ten curstomers
insert into multi_customers values(1, 1000000.23,'onewei');
insert into multi_customers values(2, 2000000.23,'twowei');
insert into multi_customers values(3, 3000000.23,'threewei');
insert into multi_customers values(4, 4000000.23,'fourwei');
insert into multi_customers values(5, 5000000.23,'fivewei');
insert into multi_customers values(6, 6000000.23,'sixwei');
insert into multi_customers values(7, 7000000.23,'sevenwei');
insert into multi_customers values(8, 8000000.23,'sixwei');
insert into multi_customers values(9, 9000000.23,'inewei');
insert into multi_customers values(10,5000000.23,'tenwei');

--insert orders
insert into multi_orders values(1,1,50000,1,123,'onedwd');
insert into multi_orders values(2,1,50001,2,123,'onedwd');

insert into multi_orders values(3,2,100000,1,123,'onedwd');
insert into multi_orders values(4,2,100001,3,123,'onedwd');

insert into multi_orders values(5,3,200000,1,123,'onedwd');
insert into multi_orders values(6,3,200001,4,123,'onedwd');

insert into multi_orders values(7,4,290000,5,123,'onedwd');
insert into multi_orders values(8,4,290001,6,123,'onedwd');

insert into multi_orders values(9,5,10000,3,123,'onedwd');
insert into multi_orders values(10,5,50001,1,123,'onedwd');

insert into multi_orders values(11,6,300001,2,123,'onedwd');
insert into multi_orders values(12,6,3000,1,123,'onedwd');

insert into multi_orders values(13,7,50000,10,123,'onedwd');
insert into multi_orders values(14,7,50009,11,123,'onedwd');

insert into multi_orders values(15,8,100001,12,123,'onedwd');
insert into multi_orders values(16,8,100001,13,123,'onedwd');

insert into multi_orders values(17,9,290001,14,123,'onedwd');
insert into multi_orders values(18,9,290000,15,123,'onedwd');

insert into multi_orders values(19,10,320000,15,123,'onedwd');
insert into multi_orders values(20,10,90000,15,123,'onedwd');

--16 rows
INSERT ALL
   WHEN order_total <= 100000 THEN
      INTO small_orders
   WHEN order_total > 1000000 AND order_total <= 200000 THEN
      INTO medium_orders
   WHEN order_total > 200000 THEN
      INTO large_orders
   SELECT order_id, order_total, sales_rep_id, customer_id
      FROM multi_orders ORDER BY order_id;

--select 9 rows
select * from small_orders order by order_id,customer_id;

--7 rows
select * from large_orders order by order_id,customer_id;

--no row
select * from special_orders order by order_id,customer_id;

--20 rows
INSERT ALL
   WHEN order_total <= 100000 THEN
      INTO small_orders
   WHEN order_total > 100000 AND order_total <= 200000 THEN
      INTO medium_orders
   ELSE
      INTO large_orders
   SELECT order_id, order_total, sales_rep_id, customer_id
      FROM multi_orders;

--select
--18 rows
select * from small_orders order by order_id,customer_id;

--14 rows
select * from large_orders order by order_id,customer_id;

--0 rows
select * from special_orders order by order_id,customer_id;

--24 rows
INSERT ALL
   WHEN ottl <= 100000 THEN
      INTO small_orders
         VALUES(oid, ottl, sid, cid)
   WHEN ottl > 100000 and ottl <= 200000 THEN
      INTO medium_orders
         VALUES(oid, ottl, sid, cid)
   WHEN ottl > 200000 THEN
      into large_orders
         VALUES(oid, ottl, sid, cid)
   WHEN ottl > 290000 THEN
      INTO special_orders
   SELECT o.order_id oid, o.customer_id cid, o.order_total ottl,
      o.sales_rep_id sid, c.credit_limit cl, c.cust_email cem
      FROM multi_orders o, multi_customers c
      WHERE o.customer_id = c.customer_id;

--27 rows
select * from small_orders order by order_id,customer_id;

--21 rows
select * from large_orders order by order_id,customer_id;

--4 rows
select * from special_orders order by order_id,customer_id;

--20 rows
INSERT FIRST
   WHEN ottl <= 100000 THEN
      INTO small_orders
         VALUES(oid, ottl, sid, cid)
   WHEN ottl > 100000 and ottl <= 200000 THEN
      INTO medium_orders
         VALUES(oid, ottl, sid, cid)
   WHEN ottl > 290000 THEN
      INTO special_orders
   WHEN ottl > 200000 THEN
      INTO large_orders
         VALUES(oid, ottl, sid, cid)
   SELECT o.order_id oid, o.customer_id cid, o.order_total ottl,
      o.sales_rep_id sid, c.credit_limit cl, c.cust_email cem
      FROM multi_orders o, multi_customers c
      WHERE o.customer_id = c.customer_id;

--36 rows
select * from small_orders order by order_id,customer_id;

--24 rows
select * from large_orders order by order_id,customer_id;

--8 rows
select * from special_orders order by order_id,customer_id;

CREATE TABLE people (
  person_id   INTEGER NOT NULL PRIMARY KEY,
  given_name  VARCHAR2(100) NOT NULL,
  family_name VARCHAR2(100) NOT NULL,
  title       VARCHAR2(20),
  birth_date  DATE
);

CREATE TABLE patients (
  patient_id          INTEGER NOT NULL PRIMARY KEY REFERENCES people (person_id),
  last_admission_date DATE
);

CREATE TABLE staff (
  staff_id   INTEGER NOT NULL PRIMARY KEY REFERENCES people (person_id),
  hired_date DATE
);

--12 rows
INSERT ALL
  /* Every one is a person */
  INTO people (person_id, given_name, family_name, title)
    VALUES (id, given_name, family_name, title)
  INTO patients (patient_id, last_admission_date)
    VALUES (id, admission_date)
  INTO staff (staff_id, hired_date)
    VALUES (id, hired_date)
  WITH names AS (
    SELECT 4 id, 'Ruth' given_name, 'Fox' family_name, 'Mrs' title,
           NULL::date hired_date, DATE'2009-12-31' admission_date
    FROM   dual UNION ALL
    SELECT 5 id, 'Isabelle' given_name, 'Squirrel' family_name, 'Miss' title , 
           NULL::date hired_date, DATE'2014-01-01' admission_date 
    FROM   dual UNION ALL 
    SELECT 6 id, 'Justin' given_name, 'Frog' family_name, 'Master' title,
           NULL::date hired_date, DATE'2015-04-22' admission_date
    FROM   dual UNION ALL
    SELECT 7 id, 'Lisa' given_name, 'Owl' family_name, 'Dr' title,
           DATE'2015-01-01' hired_date, NULL::date admission_date
    FROM   dual
  )
 SELECT * FROM names;

 --select
 --4 rows
select * from people order by person_id;

--4 rows
select * from patients order by patient_id;

delete from patients;
delete from staff;
delete from people;

--8 rows
INSERT ALL
  /* Everyone is a person, so insert all rows into people */
  WHEN 1=1 THEN
    INTO people (person_id, given_name, family_name, title)
    VALUES (id, given_name, family_name, title)
  /* Only people with an admission date are patients */
  WHEN admission_date IS NOT NULL THEN
    INTO patients (patient_id, last_admission_date)
    VALUES (id, admission_date)
  /* Only people with a hired date are staff */
  WHEN hired_date IS NOT NULL THEN
    INTO staff (staff_id, hired_date)
    VALUES (id, hired_date)
  WITH names AS (
    SELECT 4 id, 'Ruth' given_name, 'Fox' family_name, 'Mrs' title,
           NULL::date hired_date, DATE'2009-12-31' admission_date
    FROM   dual UNION ALL
    SELECT 5 id, 'Isabelle' given_name, 'Squirrel' family_name, 'Miss' title ,
           NULL::date hired_date, DATE'2014-01-01' admission_date
    FROM   dual UNION ALL
    SELECT 6 id, 'Justin' given_name, 'Frog' family_name, 'Master' title,
           NULL::date hired_date, DATE'2015-04-22' admission_date
    FROM   dual UNION ALL
    SELECT 7 id, 'Lisa' given_name, 'Owl' family_name, 'Dr' title,
           DATE'2015-01-01' hired_date, NULL::date admission_date
    FROM   dual
  )
  SELECT * FROM names;

--select
--4 rows
select * from people order by person_id;
--3 rows
select * from patients order by patient_id;
--1 row
select * from staff order by staff_id;

--clean data
drop table patients;
drop table staff;
drop table people;

drop table small_orders;
drop table large_orders;
drop table special_orders;
DROP TABLE  medium_orders;

drop table sales_input_table;
drop table sales;
DROP TABLE multi_customers;
DROP TABLE multi_orders;

--test common for self
create table t_multi(t integer,name varchar(23));
create table t1(t integer,name varchar(23) default 'ok');
create table t2(t integer,name varchar(23) default 'no');
create table t3(t integer,name varchar(23) default 'xie');
create table a_gram(id integer,name varchar(23));

insert into t_multi values(1,'one');
insert into t_multi values(2,'two');
insert into t_multi values(3,'three');
insert into t_multi values(4,'four');
insert into t_multi values(5,'five');
insert into t_multi values(6,'six');
insert into t_multi values(7,'seven');
insert into t_multi values(8,'eight');
insert into t_multi values(9,'nine');

--9 rows
--test insert all
insert all when t < 3 then
   into t1
 when  t >=3 and t < 6 then
   into t2
 when  t >= 6 then
   into t3
select * from t_multi;

--2 rows
select * from t1 order by t;

--3 rows
select * from t2 order by t;

--4 rows
select * from t3 order by t;

--9 rows
insert all when t < 3 then
   into t1(t) values(t)
 when  t >=3 and t < 6 then
   into t2(name) values(name)
 when  t >= 6 then
   into t3(t,name) values(t,name)
select * from  t_multi;

--4 rows
select * from t1 order by t,name;

--6 rows
select * from t2 order by t,name;

--8 rows
select * from t3 order by t,name;

delete from t1;
delete from t2;
delete from t3;

--9 rows
insert all when t < 3 then
   into t1(t) values(t)
 when  t >=3 and t < 6 then
   into t2(name) values(name)
 when  t >= 6 and t < 8 then
   into t3(t,name) values(t,name)
 else
	into t1
select * from  t_multi;

--4 rows
select * from t1 order by t,name;

--3 rows
select * from t2 order by t,name;

--2 rows
select * from t3 order by t,name;

delete from t1;
delete from t2;
delete from t3;

--18 rows
insert all when t < 3 then
   into t1(t) values(t)
   into t2
 when  t >=3 and t < 6 then
   into t2(name) values(name)
   into t1
 when  t >= 6 and t < 8 then
   into t3(t,name) values(t,name)
   into t2
 else
	into t1
	into t3
select * from t_multi;

--7 rows
select * from t1 order by t,name;

--7 rows
select * from t2 order by t,name;

--4 rows
select * from t3 order by t,name;

delete from t1;
delete from t2;
delete from t3;

--test first
--9 rows
insert first when t < 8 then
   into t1(t) values(t)
 when  t >=3 and t <=8 then
   into t2(name) values(name)
 when t < 9 then
	into t1
 else
	into t3
select * from  t_multi;

--7 rows
select * from t1 order by t,name;

--1 row
select * from t2 order by t,name;

-- 1 row
select * from t3 order by t,name;

DELETE FROM t1;
DELETE FROM t2;
DELETE FROM t3;

--22 rows
insert all when t < 8 then
   into t1(t) values(t)
 when  t >=3 and t <=8 then
   into t2(name) values(name)
 when t < 9 then
	into t1
 else
	into t3
select * from  t_multi;

--15 rows
select * from t1 order by t,name;

--6 rows
select * from t2 order by t,name;

--1 row
select * from t3 order by t,name;

delete from t1;
delete from t2;
delete from t3;

--21 rows
insert all when t < 8 then
   into t1(t) values(t)
 when  t >=3 and t <=8 then
   into t2(name) values(name)
 when t < 9 then
	into t1
select * from  t_multi;

--15 rows
select * from t1 order by t,name;

--6 rows
select * from t2 order by t,name;

--no row
select * from t3 order by t,name;

--clean data
drop table t_multi;
drop table t1;
drop table t2;
drop table t3;
DROP TABLE a_gram;

---not support multi table insert for view or matrivew view sequence
create table test_t (id integer,name  varchar(23));
create view t_view as select * from test_t;

--materialized view
create materialized view mater_view as select * from test_t;

--report error
insert all into t_view
  select 1,'xiexie' from dual;

--insert failed error
insert all
   into mater_view
 select 1,'name' from dual;

--sequence
CREATE SEQUENCE SEQ_DEMO
INCREMENT BY 1
START WITH 1
NOMAXvalue
NOCYCLE
NOCACHE;

--common insert ok
insert into test_t (id) values (SEQ_DEMO.nextval);

--sequence failed
insert all
  into test_t
select SEQ_DEMO.currval,'jsdjfs' from dual;

--error
insert ALL
  WHEN seq_demo.currval =1 then
  into test_t
select 'jsdjfs' from dual;

--error
insert ALL
  into test_t
select (SELECT seq_demo.currval FROM dual) AS id from dual;

--error
insert ALL
  into test_t
select (SELECT 'xiexie' AS id FROM dual WHERE seq_demo.currval = 1 )
AS id from dual;

--test foreign table and dblink
---test dblink
CREATE USER enrol11 WITH superuser;
create schema enrol11;
create table enrol11.test_remote_insert(id integer,name varchar2(256));

insert into enrol11.test_remote_insert values(1,'one');

create table enrol11.test_local_insert(id integer,name varchar2(256));

--add dblink
DO $d$
    BEGIN
        EXECUTE $$ create public database link tdblnk
			connect to enrol11 identified by "ivy123"
			using '(DESCRIPTION =
            (ADDRESS = (PROTOCOL = TCP)(PORT = $$||current_setting('ivorysql.port')||$$))
            (CONNECT_DATA =
                (SERVER = DEDICATED)
                (SERVICE_NAME = regression)
            ))'$$;
    END;
$d$;

--error
insert ALL
	into enrol11.test_remote_insert@tdblnk
select * from enrol11.test_remote_insert@tdblnk;

--ok
insert into enrol11.test_remote_insert@tdblnk select * from enrol11.test_remote_insert@tdblnk;

--ok
insert all
	into enrol11.test_local_insert
	into  enrol11.test_local_insert(id,name) values(id,name)
select * from enrol11.test_remote_insert@tdblnk;

--4 rows
select * from enrol11.test_local_insert order by id,name;

--error
insert all
	into enrol11.test_local_insert
	into  enrol11.test_local_insert(id,name) values(id,name)
	into enrol11.test_remote_insert@tdblnk
select * from enrol11.test_remote_insert@tdblnk;

--clean
drop public database link tdblnk;
drop table  enrol11.test_local_insert;
drop table enrol11.test_remote_insert;
drop schema enrol11;
drop user enrol11;

--test foreign table
--test postgres_fdw as example
DO $d$
    BEGIN
        EXECUTE $$CREATE SERVER loopback FOREIGN DATA WRAPPER postgres_fdw
            OPTIONS (dbname '$$||current_database()||$$',
                     port '$$||current_setting('ivorysql.port')||$$'
            )$$;
    END;
$d$;

CREATE USER MAPPING FOR current_user SERVER loopback;

create table test_local_insert(id integer,name varchar2(256));

CREATE FOREIGN TABLE test_foreign_table (
	id integer,
	name varchar2(256)
) SERVER loopback OPTIONS (table_name 'test_local_insert');

--ok
insert into test_foreign_table values(1,'one');

--error
INSERT
  all into test_foreign_table
select * from test_foreign_table;

--ok
insert all
 into test_local_insert
 into test_local_insert
select * from test_foreign_table;

--ok
select * from test_foreign_table order by id,name;

--error
INSERT
  all into test_foreign_table
  into test_local_insert
select * from test_foreign_table;

--clean
drop foreign table test_foreign_table;
drop table test_local_insert;
drop USER MAPPING FOR current_user SERVER loopback;
drop server loopback;


insert into test_t values(1,'sjfjsf');
insert into test_t values(2,'sjfjsf');
insert into test_t values(3,'sfjsf');
insert into test_t values(4,'sjf');
insert into test_t values(5,'sfjsf');
insert into test_t values(6,'sjf');
insert into test_t values(7,'sjfjsf');
insert into test_t values(8,'sfjsf');
insert into test_t values(9,'sjf');

create table test_1(id integer,name varchar(23));
create table test_2(id integer,name varchar(23));

--test success
--10 rows
insert all
  when mod(id,2) = 0 then
	into test_1 values(SEQ_DEMO.currval,'lll')
  else
	into test_2 values(SEQ_DEMO.currval, '222')
select * from test_t;

--4 rows
select * from test_1 order by id,name;

--6 rows
select * from test_2 order by id,name;

delete from test_1;
delete from test_2;

--oracle success
insert all
  when mod(id,2) = 0 then
	into test_1 values(SEQ_DEMO.nextval,'lll')
  else
	into test_2 values(SEQ_DEMO.nextval, '222')
select * from test_t;

--4 rows
select * from test_1 order by id,name;
--6 rows
select * from test_2 order by id,name;

create or replace function test_f(id integer) return integer is
begin
  return id;
end;
/

delete from test_1;
delete from test_2;

--oracle sucess
insert all
  when mod(id,2) = 0 then
	into test_1 values(test_f(id),'lll')
  else
	into test_2 values(test_f(id), '222')
select * from test_t;

--4 rows
select * from test_1 order by id,name;

--6 rows
select * from test_2 order by id,name;

TRUNCATE TABLE test_1;
TRUNCATE TABLE test_2;

---in plsql ok
declare
begin
  insert all
  when mod(id,2) = 0 then
	into test_1 values(SEQ_DEMO.currval,'lll')
  else
	into test_2 values(SEQ_DEMO.currval, '222')
select * from test_t;
end;
/

--4 rows
select * from test_1 order by id,name;

--6 rows
select * from test_2 order by id,name;

delete from test_1;
delete from test_2;


--success
declare
begin
  insert all
	into test_1 values(SEQ_DEMO.currval,'lll')
	into test_2 values(SEQ_DEMO.currval, '222')
select * from test_t;
end;
/

--10 rows
select * from test_1 order by id,name;

--10 rows
select * from test_2 order by id,name;


--test trigger
create or replace function before_trigger1_func return trigger is
begin
  raise info 'fire before trigger for test_1';
  return new;
end;
/

create or replace trigger before_trigger1 before insert on test_1
  execute function before_trigger1_func();
/

create or replace function after_trigger2_func return trigger is
begin
  raise info 'fire after trigger for test_2';
  return new;
end;
/

create or replace trigger after_trigger2 after insert on
test_2 execute function after_trigger2_func();
/

--20 rows
insert all
  into test_1
  into test_2
select *from test_t ORDER BY id;

--10 rows
insert all
 when 1 = 0 then
  into test_1
 else
  into test_2
select *from test_t ORDER BY id;

--clean data
DROP VIEW t_view;
DROP MATERIALIZED VIEW mater_view;
DROP TABLE test_t;
DROP TABLE test_1;
DROP TABLE test_2;
DROP FUNCTION before_trigger1_func;
DROP FUNCTION after_trigger2_func;
DROP sequence seq_demo;
DROP FUNCTION test_f;

---all trigger
create table test_t (id integer,name  varchar(23));
create index test_t_index on test_t(id);

insert into test_t values(1,'sjfjsf');
insert into test_t values(2,'sjfjsf');
insert into test_t values(3,'sfjsf');
insert into test_t values(4,'sjf');
insert into test_t values(5,'sfjsf');
insert into test_t values(6,'sjf');
insert into test_t values(7,'sjfjsf');
insert into test_t values(8,'sfjsf');
insert into test_t values(9,'sjf');

create table test_1(id integer,name varchar(23));
create index test_1_index on test_1(id);

create table test_2(id integer,name varchar(23));

create index test_2_id_index on test_2(id);
create index test_2_name_index on test_2(name);

--trigger
create or replace function before_trigger1_func return trigger is
begin
  raise info 'fire before trigger for test_1';
  return new;
end;
/

create or replace trigger before_trigger1 before insert on
test_1 execute function before_trigger1_func();
/

create or replace function after_trigger1_row_func return trigger is
begin
  raise info 'fire after insert row trigger for test_1';
  return new;
end;
/

create or replace trigger after_trigger1_row after insert on
test_1 for each row execute function after_trigger1_row_func();
/

create or replace function after_trigger2_func return trigger
is
begin
  raise info 'fire after trigger for test_2';
  return new;
end;
/

create or replace trigger after_trigger2 after insert on
test_2 execute function after_trigger2_func();
/

create or replace function before_trigger2_row_func return trigger
is
begin
   raise info 'fire before insert row trigger for test_2';
   return new;
end;
/

create or replace trigger before_trigger2_row before insert on
test_2 for each row execute function before_trigger2_row_func();
/

--18 rows
insert all
  into test_1
  into test_2
select *from test_t ORDER BY id;

--9 rows
insert all
 when 1 = 0 then
  into test_1
 else
  into test_2 values(id +1,name)
select *from test_t ORDER BY id;

--again
insert all
 when 1 = 0 then
  into test_1
 else
  into test_2 values(id +1,name)
select *from test_t ORDER BY id;

drop table test_t;
drop table test_1;
drop table test_2;
DROP FUNCTION before_trigger1_func;
DROP FUNCTION after_trigger1_row_func;
DROP FUNCTION after_trigger2_func;
DROP FUNCTION before_trigger2_row_func;

---pg mode partiton table
----
CREATE TABLE range_example(
     range_key_column integer,
     DATA VARCHAR2(20),
     ID integer
 ) PARTITION BY RANGE(range_key_column);

create table range_example_part1 partition of range_example for values
 from (minvalue) to (100);

create table range_example_part2 partition of range_example for values
 from (100) to (200);

create table range_example_part3 partition of range_example for values
 from (200) to (300);

CREATE TABLE range_example_mds(
	 data varchar2(20),
	 range_key_column integer,
	 name1 varchar2(256),
     ID integer
 ) PARTITION BY RANGE(range_key_column);

create table range_example_mds_part1 partition of range_example_mds for values
 from (minvalue) to (100);

create table range_example_mds_part2 partition of range_example_mds for values
 from (100) to (200);

create table range_example_mds_part3 partition of range_example_mds for values
 from (200) to (300);

--add index
CREATE INDEX test_index ON range_example(id);
CREATE INDEX test_index1 ON range_example_mds(data);

create table test_t(id integer,name varchar2(23));

insert into test_t values(1,'one-data');
insert into test_t values(80,'80-data');

insert into test_t values(101,'101-data');
insert into test_t values(145,'145-data');

insert into test_t values(201,'201-data');
insert into test_t values(245,'245-data');

insert into test_t values(345,'345-data');
insert into test_t values(450,'450-data');

--error
insert all
  into range_example
  into range_example values(id,name, id)
select id,name,id as total from test_t;

--12 rows
insert all
 when id < 300 then
  into range_example
  into range_example values(id,name, id)
select id,name,id as total from test_t;

--12 rows
select * from range_example order by range_key_column,DATA;

--error
insert all
  into range_example
  into range_example values(id,name, id)
  INTO range_example_mds VALUES(name,id,name,id)
select id,name,id as total from test_t;

--38 rows
insert all
 when id < 300 then
  into range_example
  into range_example values(id,name, id)
  INTO range_example_mds VALUES(name,id,name,id)
 WHEN id < 200 THEN
  INTO range_example_mds VALUES(name,id,name,id)
  into range_example values(id,name, id)
  INTO range_example_mds VALUES(name,id,name,id)
 WHEN id < 100 THEN
    into range_example values(id,name, id)
   INTO range_example_mds VALUES(name,id,name,id)
   into range_example values(id,name, id)
   INTO range_example_mds VALUES(name,id,name,id)
select id,name,id as total from test_t ORDER BY id;


--test row trigger
create or replace function before_trigger1_func return trigger is
begin
  raise info 'fire before trigger for range_example';
  return new;
end;
/

create or replace trigger before_trigger1 before insert on
range_example execute function before_trigger1_func();
/

create or replace function before_trigger1_row_func return trigger is
BEGIN
  raise info 'new=%',new;
  raise info 'fire before insert row trigger for range_example';
  return new;
end;
/

create or replace trigger before_trigger1_row before insert on
range_example for each row execute function before_trigger1_row_func();
/

create or replace function after_trigger1_row_func return trigger is
BEGIN
  raise info 'new=%',new;
  raise info 'fire after insert row trigger for range_example';
  return new;
end;
/

create or replace trigger after_trigger1_row after insert on
range_example for each row execute function after_trigger1_row_func();
/

create or replace function after_trigger1_func return trigger is
begin
  raise info 'fire after trigger for range_example';
  return new;
end;
/

create or replace trigger after_trigger1 after insert on
range_example execute function after_trigger1_func();
/

create or replace function before_trigger1_mds_func return trigger is
begin
  raise info 'fire before trigger for range_example_mds';
  return new;
end;
/

create or replace trigger before_trigger1_mds before insert on
range_example_mds execute function before_trigger1_mds_func();
/

create or replace function before_trigger1_row_mds_func return trigger is
BEGIN
  raise info 'new=%',new;
  raise info 'fire before insert row trigger for range_example_mds';
  return new;
end;
/

create or replace trigger before_trigger1_mds_row before insert on
range_example_mds for each row execute function before_trigger1_row_mds_func();
/

create or replace function after_trigger1_row_mds_func return trigger is
BEGIN
  raise info 'new=%',new;
  raise info 'fire after insert row trigger for range_example_mds';
  return new;
end;
/

create or replace trigger after_trigger1_mds_row after insert on
range_example_mds for each row execute function after_trigger1_row_mds_func();
/

create or replace function after_trigger1_mds_func return trigger is
begin
  raise info 'fire after trigger for range_example_mds';
  return new;
end;
/

create or replace trigger after_trigger1_mds after insert on
range_example_mds execute function after_trigger1_mds_func();
/

--38 rows
insert all
 when id < 300 then
  into range_example
  into range_example values(id,name, id)
  INTO range_example_mds VALUES(name,id,name,id)
 WHEN id < 200 THEN
  INTO range_example_mds VALUES(name,id,name,id)
  into range_example values(id,name, id)
  INTO range_example_mds VALUES(name,id,name,id)
 WHEN id < 100 THEN
    into range_example values(id,name, id)
   INTO range_example_mds VALUES(name,id,name,id)
   into range_example values(id,name, id)
   INTO range_example_mds VALUES(name,id,name,id)
select id,name,id as total from test_t ORDER BY id;


--subtrans
begin;
insert all
 when id < 300 then
  into range_example
  into range_example values(id,name, id)
  INTO range_example_mds VALUES(name,id,name,id)
 WHEN id < 200 THEN
  INTO range_example_mds VALUES(name,id,name,id)
  into range_example values(id,name, id)
  INTO range_example_mds VALUES(name,id,name,id)
 WHEN id < 100 THEN
    into range_example values(id,name, id)
   INTO range_example_mds VALUES(name,id,name,id)
   into range_example values(id,name, id)
   INTO range_example_mds VALUES(name,id,name,id)
select id,name,id as total from test_t ORDER BY id;
commit;

--clean data
DROP TABLE range_example;
DROP TABLE range_example_mds;
DROP TABLE test_t;
DROP FUNCTION after_trigger1_mds_func;
DROP FUNCTION after_trigger1_row_mds_func;
DROP FUNCTION before_trigger1_row_mds_func;
DROP FUNCTION before_trigger1_mds_func;
DROP FUNCTION after_trigger1_func;
DROP FUNCTION after_trigger1_row_func;
DROP FUNCTION before_trigger1_row_func;
DROP FUNCTION before_trigger1_func;


--test trigger invoker
create table test_id1(id integer primary key,name varchar2(256));
create table test_id2 (id integer primary key references test_id1(id),
		name varchar2(256));

create table source_tab(id integer primary key,name varchar2(256));

insert into source_tab values(1,'one');

insert into source_tab values(2,'two');


CREATE OR replace FUNCTION test_stmt_trigger_func RETURN TRIGGER IS
BEGIN
  raise info 'after insert on test_id1';
  RETURN new;
end;
/

create or replace trigger test_stmt_trigger after insert on test_id1
EXECUTE FUNCTION test_stmt_trigger_func();
/

CREATE OR REPLACE FUNCTION test_row_trigger_func RETURN TRIGGER IS
BEGIN
  raise info 'after insert on test_id1 row';
  RETURN new;
end;
/

create or replace trigger test_row_trigger after insert on test_id1 for
each row EXECUTE FUNCTION test_row_trigger_func();
/

CREATE OR REPLACE FUNCTION test_stmt1_trigger_func RETURN TRIGGER IS
BEGIN
  raise info 'after insert on test_id2';
  RETURN new;
end;
/

create or replace trigger test_stmt1_trigger after insert on test_id2
EXECUTE FUNCTION test_stmt1_trigger_func();
/

CREATE OR REPLACE FUNCTION test_row1_trigger_func RETURN TRIGGER IS
BEGIN
   raise info 'after insert on test_id2 row';
   RETURN new;
end;
/

create or replace trigger test_row1_trigger after insert on test_id2 for
each row EXECUTE FUNCTION test_row1_trigger_func();
/

--4 rows
insert all
  into test_id1 values(id,name)
  into test_id2 values(id,name)
select * from source_tab ORDER BY id;

DELETE FROM test_id2;
DELETE FROM test_id1;
DROP TRIGGER test_row1_trigger ON test_id2;
DROP TRIGGER test_stmt1_trigger ON test_id2;
DROP TRIGGER test_row_trigger ON test_id1;
DROP TRIGGER test_stmt_trigger ON test_id1;
DROP FUNCTION test_row1_trigger_func;
DROP FUNCTION test_stmt1_trigger_func;
DROP FUNCTION test_row_trigger_func;
DROP FUNCTION test_stmt_trigger_func;


CREATE OR REPLACE FUNCTION test_stmt_trigger_func RETURN TRIGGER IS
BEGIN
  raise info '%','before insert on test_id1';
  RETURN new;
end;
/

create or replace trigger test_stmt_trigger before insert on test_id1
EXECUTE FUNCTION test_stmt_trigger_func();
/

CREATE OR REPLACE FUNCTION test_row_trigger_func RETURN TRIGGER IS
BEGIN
  raise info '%','before insert on test_id1 row';
  RETURN new;
end;
/

create or replace trigger test_row_trigger before insert on test_id1 for
each row EXECUTE FUNCTION test_row_trigger_func();
/

CREATE OR REPLACE FUNCTION test_stmt1_trigger_func RETURN TRIGGER IS
BEGIN
  raise info '%','before insert on test_id2';
  RETURN new;
end;
/

create or replace trigger test_stmt1_trigger before insert on test_id2
EXECUTE FUNCTION test_stmt1_trigger_func();
/

CREATE OR REPLACE FUNCTION test_row1_trigger_func RETURN TRIGGER IS
BEGIN
  raise info '%','before insert on test_id2 row';
  RETURN new;
end;
/

create or replace trigger test_row1_trigger before insert on test_id2 for
each row EXECUTE FUNCTION test_row1_trigger_func();
/

--4 rows
insert all
  into test_id1 values(id,name)
  into test_id2 values(id,name)
select * from source_tab ORDER BY id;

DELETE FROM test_id2;
DELETE FROM test_id1;

DROP TRIGGER test_stmt_trigger ON test_id1;
DROP TRIGGER test_row_trigger ON test_id1;
DROP TRIGGER test_stmt1_trigger ON test_id2;
DROP TRIGGER test_row1_trigger ON test_id2;

DROP function test_stmt_trigger_func;
DROP function test_row_trigger_func;
DROP function test_stmt1_trigger_func;
DROP function test_row1_trigger_func;


CREATE OR REPLACE FUNCTION test_if(id integer) RETURN integer IS
BEGIN
  raise info 'invoke func';
  RETURN id;
end;
/

--2 rows
insert first
  WHEN test_if(id) = id then
	into test_id1 values(id,name)
  WHEN test_if(id) = id + 1 then
	into test_id2 values(id,name)
select * from source_tab ORDER BY id;


DELETE FROM test_id1;
DELETE FROM test_id2;

--ok
--2 rows
insert first
  WHEN test_if(id) = id then
	into test_id1 values(id,name)
  WHEN test_if(id) = id + 1 then
	into test_id2 values(id,name)
(select * from source_tab ORDER BY id);

DELETE FROM test_id1;
DELETE FROM test_id2;

--2 rows
insert first
  WHEN test_if(id) = id then
	into test_id1 values(id,name)
  WHEN test_if(id) = id + 1 then
	into test_id2 values(id,name)
(select * from source_tab ORDER by id);

DELETE FROM test_id1;
DELETE FROM test_id2;

--2 rows
insert all
  WHEN test_if(id) = id then
	into test_id1 values(id,name)
  WHEN test_if(id) = id + 1 then
	into test_id2 values(id,name)
select * from source_tab order by id;

--error
insert all
  WHEN test_if(id) = id then
	into test_id1 values(id,name)
  WHEN test_if(id) = id + 1 then
	into test_id2 values(id,name)
(select * from source_tab order by id);


--error
insert first
	into test_id2
(select * from source_tab);

--error
insert all
	into test_id2
(select * from source_tab);

--clean data
DROP TABLE test_id2;
DROP TABLE test_id1;
DROP TABLE source_tab;
DROP FUNCTION test_if;

--test rule
CREATE TABLE test_mds1(id integer,name varchar2(256));
CREATE TABLE test_mds2(id integer,name varchar2(256));
CREATE TABLE test_mds3(id integer,name varchar2(256));

CREATE TABLE test_insert_rule1 AS SELECT * FROM test_mds1;
CREATE TABLE test_insert_rule2 AS SELECT * FROM test_mds2;
CREATE TABLE test_insert_rule3 AS SELECT * FROM test_mds3;

CREATE TABLE test_source(id integer,name varchar2(256));

INSERT INTO test_source VALUES(1,'one');
INSERT INTO test_source VALUES(2,'two');
INSERT INTO test_source VALUES(3,'three');


--test rule
CREATE RULE test_rule1 AS ON INSERT to test_mds1
do also
   INSERT INTO test_insert_rule1 VALUES(new.id,new.name);

CREATE RULE test_rule1 AS ON INSERT to test_mds2
do also
   INSERT INTO test_insert_rule2 VALUES(new.id,new.name);

CREATE RULE test_rule1 AS ON INSERT to test_mds3
do also
   INSERT INTO test_insert_rule3 VALUES(new.id,new.name);

--error
INSERT first
  WHEN id <= 1 THEN
      INTO test_mds1(id,name) VALUES(id,name)
  WHEN id > 2 THEN
      INTO test_mds3(id,name) VALUES(id,name)
  ELSE
      INTO test_mds2(id,name) VALUES(id,name)
(SELECT * FROM test_source);

DROP RULE test_rule1 ON test_mds1;
DROP RULE test_rule1 ON test_mds2;
DROP RULE test_rule1 ON test_mds3;


CREATE RULE test_rule1 AS ON INSERT to test_mds1
do INSTEAD
   INSERT INTO test_insert_rule1 VALUES(new.id,new.name);

CREATE RULE test_rule1 AS ON INSERT to test_mds2
do INSTEAD
   INSERT INTO test_insert_rule2 VALUES(new.id,new.name);

CREATE RULE test_rule1 AS ON INSERT to test_mds3
do INSTEAD
   INSERT INTO test_insert_rule3 VALUES(new.id,new.name);

--ok
INSERT first
  WHEN id <= 1 THEN
      INTO test_mds1(id,name) VALUES(id,name)
  WHEN id > 2 THEN
      INTO test_mds3(id,name) VALUES(id,name)
  ELSE
      INTO test_mds2(id,name) VALUES(id,name)
(SELECT * FROM test_source);

--no row
SELECT * FROM test_mds1;

--1 row
SELECT * from test_insert_rule1;

--0 row
SELECT * from test_mds2;

--1 row
SELECT * FROM  test_insert_rule2;

--0 row
SELECT * FROM test_mds3;

--1 row
SELECT * FROM test_insert_rule3;

DELETE FROM test_insert_rule1;
DELETE FROM test_insert_rule2;
DELETE FROM test_insert_rule3;


INSERT first
  WHEN id <= 1 THEN
      INTO test_mds1(id,name) VALUES(id,name)
	  INTO test_mds1(id,name) VALUES(id,name)
	  INTO test_mds1(id,name) VALUES(id,name)
  WHEN id > 2 THEN
      INTO test_mds3(id,name) VALUES(id,name)
	  INTO test_mds3(id,name) VALUES(id,name)
	  INTO test_mds3(id,name) VALUES(id,name)
  ELSE
      INTO test_mds2(id,name) VALUES(id,name)
	  INTO test_mds2(id,name) VALUES(id,name)
	  INTO test_mds2(id,name) VALUES(id,name)
(SELECT * FROM test_source);


--no row
SELECT * FROM test_mds1 order by id,name;

--3 row
SELECT * from test_insert_rule1 order by id,name;

--0 row
SELECT * from test_mds2 order by id,name;

--3 row
SELECT * FROM  test_insert_rule2 order by id,name;

--0 row
SELECT * FROM test_mds3 order by id,name;

--3 row
SELECT * FROM test_insert_rule3 order by id,name;


DROP RULE test_rule1 ON test_mds1;
DROP RULE test_rule1 ON test_mds2;
DROP RULE test_rule1 ON test_mds3;

DELETE FROM test_insert_rule1;
DELETE FROM test_insert_rule2;
DELETE FROM test_insert_rule3;

CREATE RULE test_rule1 AS ON INSERT to test_mds1
do NOTHING;

CREATE RULE test_rule1 AS ON INSERT to test_mds2
do NOTHING;

CREATE RULE test_rule1 AS ON INSERT to test_mds3
do NOTHING;

--3 row
INSERT first
  WHEN id <= 1 THEN
      INTO test_mds1(id,name) VALUES(id,name)
  WHEN id > 2 THEN
      INTO test_mds3(id,name) VALUES(id,name)
  ELSE
      INTO test_mds2(id,name) VALUES(id,name)
(SELECT * FROM test_source);

--1 row
SELECT * FROM test_mds1;

--0 row
SELECT * from test_insert_rule1;

--1 row
SELECT * from test_mds2;

--0 row
SELECT * FROM  test_insert_rule2;

--1 row
SELECT * FROM test_mds3;

--0 row
SELECT * FROM test_insert_rule3;

--ok
INSERT all
  WHEN id <= 1 THEN
      INTO test_mds1(id,name) VALUES(id,name)
	  INTO test_mds1(id,name) VALUES(id,name)
	  INTO test_mds1(id,name) VALUES(id,name)
  WHEN id > 2 THEN
      INTO test_mds3(id,name) VALUES(id,name)
	  INTO test_mds3(id,name) VALUES(id,name)
	  INTO test_mds3(id,name) VALUES(id,name)
  ELSE
      INTO test_mds2(id,name) VALUES(id,name)
	  INTO test_mds2(id,name) VALUES(id,name)
	  INTO test_mds2(id,name) VALUES(id,name)
(SELECT * FROM test_source);

--4 row
SELECT * FROM test_mds1 order by id,name;

--0 row
SELECT * from test_insert_rule1 order by id,name;

--4 row
SELECT * from test_mds2 order by id,name;

--0 row
SELECT * FROM  test_insert_rule2 order by id,name;

--4 row
SELECT * FROM test_mds3 order by id,name;

--0 row
SELECT * FROM test_insert_rule3 order by id,name;

DROP RULE test_rule1 ON test_mds1;
DROP RULE test_rule1 ON test_mds2;
DROP RULE test_rule1 ON test_mds3;

DELETE FROM test_insert_rule1;
DELETE FROM test_insert_rule2;
DELETE FROM test_insert_rule3;
DELETE FROM test_mds1;
DELETE FROM test_mds2;
DELETE FROM test_mds3;

--test select
CREATE RULE test_rule1 AS ON INSERT to test_mds1
do ALSO
SELECT * FROM test_insert_rule1;

CREATE RULE test_rule1 AS ON INSERT to test_mds2
do ALSO
SELECT * FROM test_insert_rule2;

CREATE RULE test_rule1 AS ON INSERT to test_mds3
do ALSO
SELECT * FROM test_insert_rule2;

--error
INSERT first
  WHEN id <= 1 THEN
      INTO test_mds1(id,name) VALUES(id,name)
  WHEN id > 2 THEN
      INTO test_mds3(id,name) VALUES(id,name)
  ELSE
      INTO test_mds2(id,name) VALUES(id,name)
(SELECT * FROM test_source);

DROP RULE test_rule1 ON test_mds1;
DROP RULE test_rule1 ON test_mds2;
DROP RULE test_rule1 ON test_mds3;

CREATE RULE test_rule1 AS ON INSERT to test_mds1
do INSTEAD
SELECT * FROM test_insert_rule1;

CREATE RULE test_rule1 AS ON INSERT to test_mds2
do INSTEAD
SELECT * FROM test_insert_rule2;

CREATE RULE test_rule1 AS ON INSERT to test_mds3
do INSTEAD
SELECT * FROM test_insert_rule2;

--ok no row
INSERT first
  WHEN id <= 1 THEN
      INTO test_mds1(id,name) VALUES(id,name)
  WHEN id > 2 THEN
      INTO test_mds3(id,name) VALUES(id,name)
  ELSE
      INTO test_mds2(id,name) VALUES(id,name)
(SELECT * FROM test_source);



INSERT all
  WHEN id <= 1 THEN
      INTO test_mds1(id,name) VALUES(id,name)
	  INTO test_mds1(id,name) VALUES(id,name)
	  INTO test_mds1(id,name) VALUES(id,name)
  WHEN id > 2 THEN
      INTO test_mds3(id,name) VALUES(id,name)
	  INTO test_mds3(id,name) VALUES(id,name)
	  INTO test_mds3(id,name) VALUES(id,name)
  ELSE
      INTO test_mds2(id,name) VALUES(id,name)
	  INTO test_mds2(id,name) VALUES(id,name)
	  INTO test_mds2(id,name) VALUES(id,name)
(SELECT * FROM test_source);

--drop
DROP RULE test_rule1 ON test_mds1;
DROP RULE test_rule1 ON test_mds2;
DROP RULE test_rule1 ON test_mds3;


--add when
DELETE FROM test_insert_rule1;
DELETE FROM test_insert_rule2;
DELETE FROM test_insert_rule3;
DELETE FROM test_mds1;
DELETE FROM test_mds2;
DELETE FROM test_mds3;

CREATE RULE test_rule1 AS ON INSERT to test_mds1 WHERE (new.id = 2)
do INSTEAD
   INSERT INTO test_insert_rule1 VALUES(new.id,new.name);

CREATE RULE test_rule1 AS ON INSERT to test_mds2 WHERE (new.id != 2)
do INSTEAD
   INSERT INTO test_insert_rule2 VALUES(new.id,new.name);

CREATE RULE test_rule1 AS ON INSERT to test_mds3 WHERE (new.id != 3)
do INSTEAD
   INSERT INTO test_insert_rule3 VALUES(new.id,new.name);

--error
INSERT all
  WHEN id <= 1 THEN
      INTO test_mds1(id,name) VALUES(id,name)
  WHEN id > 2 THEN
      INTO test_mds3(id,name) VALUES(id,name)
  ELSE
      INTO test_mds2(id,name) VALUES(id,name)
(SELECT * FROM test_source);

--error
INSERT first
  WHEN id <= 1 THEN
      INTO test_mds1(id,name) VALUES(id,name)
  WHEN id > 2 THEN
      INTO test_mds3(id,name) VALUES(id,name)
  ELSE
      INTO test_mds2(id,name) VALUES(id,name)
(SELECT * FROM test_source);


DROP RULE test_rule1 ON test_mds1;
DROP RULE test_rule1 ON test_mds2;
DROP RULE test_rule1 ON test_mds3;


--more action
CREATE RULE test_rule1 AS ON INSERT to test_mds1
do INSTEAD
(SELECT * FROM test_insert_rule1;
SELECT * FROM test_insert_rule2;
;);

CREATE RULE test_rule1 AS ON INSERT to test_mds2
do INSTEAD
(SELECT * FROM test_insert_rule2;
SELECT * FROM test_insert_rule1;);

CREATE RULE test_rule1 AS ON INSERT to test_mds3
do INSTEAD
(SELECT * FROM test_insert_rule2;;);

--error
INSERT first
  WHEN id <= 1 THEN
      INTO test_mds1(id,name) VALUES(id,name)
  WHEN id > 2 THEN
      INTO test_mds3(id,name) VALUES(id,name)
  ELSE
      INTO test_mds2(id,name) VALUES(id,name)
(SELECT * FROM test_source);


--error
INSERT all
  WHEN id <= 1 THEN
      INTO test_mds1(id,name) VALUES(id,name)
	  INTO test_mds1(id,name) VALUES(id,name)
  WHEN id > 2 THEN
      INTO test_mds3(id,name) VALUES(id,name)
	  INTO test_mds3(id,name) VALUES(id,name)
  ELSE
      INTO test_mds2(id,name) VALUES(id,name)
	  INTO test_mds2(id,name) VALUES(id,name)
(SELECT * FROM test_source);


--clean data
drop TABLE test_mds1;
drop TABLE test_mds2;
drop TABLE test_mds3;

drop TABLE test_insert_rule1;
drop TABLE test_insert_rule2;
drop TABLE test_insert_rule3;
drop TABLE test_source;

---test when true
--else
CREATE TABLE test_when_source(id integer,name varchar2(256));
CREATE TABLE test_dest_target1(id integer,name varchar2(256));
CREATE TABLE test_dest_target2(id integer,name varchar2(256));
CREATE TABLE test_dest_target3(id integer,name varchar2(256));

INSERT INTO test_when_source VALUES(1,'one');
INSERT INTO test_when_source VALUES(2,'two');
INSERT INTO test_when_source VALUES(3,'three');

-- 3 rows
INSERT ALL
  WHEN 1=1 THEN
     INTO test_dest_target1
  ELSE
     INTO test_dest_target2
SELECT  *FROM test_when_source;

--3
SELECT COUNT(*) FROM test_dest_target1;
--0
SELECT COUNT(*) FROM test_dest_target2;

--3 rows
INSERT ALL
  WHEN 1=2 THEN
     INTO test_dest_target1
  ELSE
     INTO test_dest_target2
SELECT * FROM test_when_source;

--3
SELECT COUNT(*) FROM test_dest_target1;

--3
SELECT COUNT(*) FROM test_dest_target2;

--3 rows inserted
INSERT first
  WHEN 1=0 THEN
     INTO test_dest_target1
  WHEN 1 = 1 THEN
     INTO test_dest_target3
  ELSE
     INTO test_dest_target2
SELECT  *FROM test_when_source;

--3
SELECT COUNT(*) FROM test_dest_target1;

--3
SELECT COUNT(*) FROM test_dest_target2;

--3
SELECT COUNT(*) FROM test_dest_target3;

--3 rows
INSERT first
  WHEN 1=1 THEN
     INTO test_dest_target1
  WHEN 1 = 1 THEN
     INTO test_dest_target3
  ELSE
     INTO test_dest_target2
SELECT  *FROM test_when_source;

--6
SELECT COUNT(*) FROM test_dest_target1;

--3
SELECT COUNT(*) FROM test_dest_target2;

--3
SELECT COUNT(*) FROM test_dest_target3;

--6 rows
INSERT all
  WHEN 1=1 THEN
     INTO test_dest_target1
  WHEN 1 = 1 THEN
     INTO test_dest_target3
  ELSE
     INTO test_dest_target2
SELECT  *FROM test_when_source;

--9
SELECT COUNT(*) FROM test_dest_target1;

--3
SELECT COUNT(*) FROM test_dest_target2;

--6
SELECT COUNT(*) FROM test_dest_target3;

--3 row inserted
INSERT all
  WHEN 1=1 THEN
     INTO test_dest_target1
  WHEN 1 = 0 THEN
     INTO test_dest_target3
  ELSE
     INTO test_dest_target2
SELECT  *FROM test_when_source;

--12
SELECT COUNT(*) FROM test_dest_target1;

--3
SELECT COUNT(*) FROM test_dest_target2;

--6
SELECT COUNT(*) FROM test_dest_target3;

--3 rows
INSERT first
  WHEN 1=1 THEN
     INTO test_dest_target1
  WHEN 1 = 0 THEN
     INTO test_dest_target3
  ELSE
     INTO test_dest_target2
SELECT  *FROM test_when_source;

--15
SELECT COUNT(*) FROM test_dest_target1;

--3
SELECT COUNT(*) FROM test_dest_target2;

--6
SELECT COUNT(*) FROM test_dest_target3;

--3 rows inserted
INSERT first
  WHEN 1=2 THEN
     INTO test_dest_target1
  WHEN 1 = 0 THEN
     INTO test_dest_target3
  ELSE
     INTO test_dest_target2
SELECT  *FROM test_when_source;

--15
SELECT COUNT(*) FROM test_dest_target1;

--6
SELECT COUNT(*) FROM test_dest_target2;

--6
SELECT COUNT(*) FROM test_dest_target3;

--3 row inserted
INSERT all
  WHEN 1=2 THEN
     INTO test_dest_target1
  WHEN 1 = 0 THEN
     INTO test_dest_target3
  ELSE
     INTO test_dest_target2
SELECT  *FROM test_when_source;

--15
SELECT COUNT(*) FROM test_dest_target1;

--9
SELECT COUNT(*) FROM test_dest_target2;

--6
SELECT COUNT(*) FROM test_dest_target3;


--6 rows inserted
INSERT all
  WHEN 1=2 THEN
     INTO test_dest_target1
  WHEN 1 = 0 THEN
     INTO test_dest_target3
  ELSE
     INTO test_dest_target2
SELECT  *FROM test_when_source UNION
ALL
select * from test_when_source;

--error
INSERT all
  WHEN 1=2 THEN
     INTO test_dest_target1
  WHEN 1 = 0 THEN
     INTO test_dest_target3
  ELSE
     INTO test_dest_target2
(VALUES(1,2));

--test preapre
prepare test_mulit_insert (integer,varchar2) AS
   INSERT all
  WHEN 1=2 THEN
     INTO test_dest_target1
  WHEN 1 = 0 THEN
     INTO test_dest_target3
  ELSE
     INTO test_dest_target2
SELECT  *FROM test_when_source UNION
ALL
  SELECT $1,$2;

\set EXECUTE_RUN_PREPARE ON

--ok
EXECUTE test_mulit_insert(1,'xiexie');
EXECUTE test_mulit_insert(2,'mds');

DEALLOCATE test_mulit_insert;

--test select && target
DELETE FROM test_dest_target1;
DELETE FROM test_dest_target2;
DELETE FROM test_dest_target3;

INSERT INTO test_dest_target1 VALUES(1,'one');
INSERT INTO test_dest_target1 VALUES(2,'two');
INSERT INTO test_dest_target2 VALUES(3,'three');
INSERT INTO test_dest_target2 VALUES(4,'four');
INSERT INTO test_dest_target3 VALUES(5,'five');
INSERT INTO test_dest_target3 VALUES(6,'six');

--6 rows
INSERT first
  WHEN id < 3 THEN
     INTO test_dest_target1
  WHEN id < 5 THEN
     INTO test_dest_target2
  else
     INTO test_dest_target3
  SELECT * FROM test_dest_target1
  UNION ALL
  SELECT * FROM test_dest_target2
  UNION ALL
  SELECT * FROM test_dest_target3;

--4 rows
SELECT * FROM test_dest_target1 ORDER BY id;
--4 rows
SELECT * FROM test_dest_target2 ORDER BY id;
--4 rows
SELECT * FROM test_dest_target3 ORDER BY id;

--multi insert global temp table
create global temporary table temp1_session(a integer,name varchar2(256)) ON COMMIT PRESERVE ROWS;
create global temporary table temp1_trans(a integer,name varchar2(256)) ON COMMIT delete ROWS;

--24 rows
INSERT all
  INTO temp1_session
  INTO temp1_trans
  SELECT * FROM test_dest_target1
  UNION ALL
  SELECT * FROM test_dest_target2
  UNION ALL
  SELECT * FROM test_dest_target3;

--12 rows
SELECT * FROM temp1_session ORDER BY a;
--0 rows
SELECT * FROM temp1_trans ORDER BY a;

--clean data
TRUNCATE TABLE temp1_session;
DROP TABLE temp1_session;
DROP TABLE temp1_trans;

DROP TABLE test_dest_target1;
DROP TABLE test_dest_target2;
DROP TABLE test_dest_target3;
DROP TABLE test_when_source;

--test policy
SET ivorysql.enable_ora_user_schema = on;
CREATE USER test_insert_user NOLOGIN;
SET SESSION AUTHORIZATION test_insert_user;

CREATE TABLE test_when_source(id integer,name varchar2(256));
CREATE TABLE test_dest_target1(id integer,name varchar2(256));
CREATE TABLE test_dest_target2(id integer,name varchar2(256));

CREATE TABLE test_dest_target3(id integer,name varchar2(256));

INSERT INTO test_when_source VALUES(1,'one');
INSERT INTO test_when_source VALUES(2,'two');
INSERT INTO test_when_source VALUES(3,'three');

---test_when_source add select policy
--test_dest add into target
DELETE FROM test_dest_target1;
DELETE FROM test_dest_target2;
DELETE FROM test_dest_target3;

--add select policy from test_when_source
ALTER TABLE test_when_source enable ROW LEVEL SECURITY;
ALTER TABLE test_dest_target1 enable ROW LEVEL SECURITY;
ALTER TABLE test_dest_target2 enable ROW LEVEL SECURITY;
ALTER TABLE test_dest_target3 enable ROW LEVEL SECURITY;

ALTER TABLE test_when_source force ROW LEVEL SECURITY;
ALTER TABLE test_dest_target1 force ROW LEVEL SECURITY;
ALTER TABLE test_dest_target2 force ROW LEVEL SECURITY;
ALTER TABLE test_dest_target3 force ROW LEVEL SECURITY;

CREATE policy test_source_policy1 ON test_when_source
FOR SELECT TO CURRENT_USER
using (true);

CREATE TABLE test_when_source1 AS SELECT * FROM test_when_source;

CREATE policy test_source_policy ON test_when_source
FOR SELECT TO CURRENT_USER
using (id IN (SELECT id FROM test_when_source1));

--test with check
CREATE policy test_target1_policy ON test_dest_target1 FOR INSERT WITH CHECK
(id < 10000);

CREATE policy test_target2_policy ON test_dest_target2 FOR INSERT WITH
CHECK (id IN (SELECT id FROM test_when_source));

CREATE policy test_target3_policy ON test_dest_target3 FOR INSERT WITH
CHECK (id IN (SELECT id FROM test_when_source1));

CREATE policy test_target1_policy1 ON test_dest_target1
FOR SELECT TO CURRENT_USER
using (id < 1000);

CREATE policy test_target2_policy1 ON test_dest_target2
FOR SELECT TO CURRENT_USER
using (id < 1000);

CREATE policy test_target3_policy1 ON test_dest_target3
FOR SELECT TO CURRENT_USER
using (id < 1000);

--9 rows
INSERT all
     INTO test_dest_target1
     INTO test_dest_target3
     INTO test_dest_target2
SELECT  *FROM test_when_source ORDER BY id;

--3 rows
SELECT * FROM test_dest_target1 ORDER BY id;
--3 rows
SELECT * FROM test_dest_target2 ORDER BY id;
--3 rows
SELECT * FROM test_dest_target3 ORDER BY id;

--3 rows
INSERT first
   WHEN id < 2 then
     INTO test_dest_target1
   WHEN id < 3 then
     INTO test_dest_target3
   else
     INTO test_dest_target2
SELECT  *FROM test_when_source ORDER BY id;

DROP policy test_target1_policy ON test_dest_target1;

CREATE policy test_target1_policy ON test_dest_target1 FOR INSERT WITH CHECK
(id = 3);

--raise error
INSERT first
   WHEN id < 2 then
     INTO test_dest_target1
   WHEN id < 3 then
     INTO test_dest_target3
   else
     INTO test_dest_target2
SELECT  *FROM test_when_source ORDER BY id;

DROP policy test_target1_policy ON test_dest_target1;

--error
INSERT first
   WHEN id < 2 then
     INTO test_dest_target1
   WHEN id < 3 then
     INTO test_dest_target3
   else
     INTO test_dest_target2
SELECT  *FROM test_when_source ORDER BY id;

CREATE policy test_target1_policy ON test_dest_target1 FOR INSERT WITH CHECK
(id < 3);

--ok
--3 rows
INSERT first
   WHEN id < 2 then
     INTO test_dest_target1
   WHEN id < 3 then
     INTO test_dest_target3
   else
     INTO test_dest_target2
SELECT  *FROM test_when_source ORDER BY id;

--5 rows
SELECT * FROM test_dest_target1 ORDER BY id;
--5 rows
SELECT * FROM test_dest_target2 ORDER BY id;
--5 rows
SELECT * FROM test_dest_target3 ORDER BY id;

SET row_security = on;

prepare test_mulit_insert (integer,varchar2) AS
INSERT ALL
   WHEN id < 3 then
     INTO test_dest_target1 values($1,$2)
	 INTO test_dest_target1 VALUES($1,$2)
	 into test_dest_target1
   else
     INTO test_dest_target3
     INTO test_dest_target2
SELECT  *FROM test_when_source UNION
ALL
  SELECT $1,$2;

\set EXECUTE_RUN_PREPARE ON

--ok
EXECUTE test_mulit_insert(1,'xiexie');
EXECUTE test_mulit_insert(2,'mds');

DEALLOCATE test_mulit_insert;

--add with
--3 rows
WITH a AS (SELECT * FROM test_when_source ORDER BY id)
INSERT first
   WHEN id < 2 then
     INTO test_dest_target1
   WHEN id < 3 then
     INTO test_dest_target3
   else
     INTO test_dest_target2
SELECT  *FROM a;

---all test_target add another policy
CREATE OR REPLACE FUNCTION test_f(id integer) RETURN integer IS
BEGIN
  raise info 'test_f id:%', id;
  RETURN id;
end;
/

CREATE policy test_target1_policy3 ON test_dest_target1 AS RESTRICTIVE FOR INSERT WITH CHECK
(id < 2000);

CREATE policy test_target2_policy3 ON test_dest_target2 FOR INSERT WITH
CHECK (id NOT IN (SELECT id FROM test_when_source));

CREATE policy test_target3_policy3 ON test_dest_target3 AS RESTRICTIVE FOR INSERT WITH
CHECK (id  = test_f(id));

--ok
WITH a AS (SELECT * FROM test_when_source ORDER BY id)
INSERT first
   WHEN id < 2 then
     INTO test_dest_target1
   else
     INTO test_dest_target3
     INTO test_dest_target2
SELECT  *FROM a;

--clean data
DROP TABLE test_dest_target1;
DROP TABLE test_dest_target2;
DROP TABLE test_dest_target3;
DROP TABLE test_when_source;
DROP TABLE test_when_source1;
DROP FUNCTION test_f;

--partiton table has row security
--test_when_source
CREATE TABLE test_when_source(id integer,name varchar2(256)) PARTITION BY range(id);

CREATE TABLE test_when_source_part1 PARTITION OF test_when_source
  FOR VALUES FROM (minvalue) TO (0) PARTITION BY list (name);

CREATE TABLE test_when_source_part11 PARTITION OF test_when_source_part1 FOR VALUES IN ('a');
CREATE TABLE test_when_source_part12 PARTITION OF test_when_source_part1 FOR VALUES IN ('b');
CREATE TABLE test_when_source_part13 PARTITION OF test_when_source_part1 FOR VALUES IN ('c');

CREATE TABLE test_when_source_part2 PARTITION OF test_when_source
  FOR VALUES FROM (0) TO (100) PARTITION BY list (name);

CREATE TABLE test_when_source_part21 PARTITION OF test_when_source_part2 FOR VALUES IN ('a');
CREATE TABLE test_when_source_part22 PARTITION OF test_when_source_part2 FOR VALUES IN ('b');
CREATE TABLE test_when_source_part23 PARTITION OF test_when_source_part2 FOR VALUES IN ('c');

CREATE TABLE test_when_source_part3 PARTITION OF test_when_source
  FOR VALUES FROM (100) TO (200) partition BY list(name);
CREATE TABLE test_when_source_part31 PARTITION OF test_when_source_part3 FOR VALUES IN ('a');
CREATE TABLE test_when_source_part32 PARTITION OF test_when_source_part3 FOR VALUES IN ('b');
CREATE TABLE test_when_source_part33 PARTITION OF test_when_source_part3 FOR VALUES IN ('c');

CREATE TABLE test_when_source_part4 PARTITION OF test_when_source
  FOR VALUES FROM (200) TO (maxvalue) PARTITION BY list (name);

CREATE TABLE test_when_source_part41 PARTITION OF test_when_source_part4 FOR VALUES IN ('a');
CREATE TABLE test_when_source_part42 PARTITION OF test_when_source_part4 FOR VALUES IN ('b');
CREATE TABLE test_when_source_part43 PARTITION OF test_when_source_part4 FOR VALUES IN ('c');

CREATE INDEX test_index ON test_when_source(id);

--test_dest_target1
CREATE TABLE test_dest_target1(id integer,name varchar2(256)) PARTITION BY range(id);

CREATE TABLE test_dest_target1_part1 PARTITION OF test_dest_target1
  FOR VALUES FROM (minvalue) TO (0) PARTITION BY list (name);

CREATE TABLE test_dest_target1_part11 PARTITION OF test_dest_target1_part1 FOR VALUES IN ('a');
CREATE TABLE test_dest_target1_part12 PARTITION OF test_dest_target1_part1 FOR VALUES IN ('b');
CREATE TABLE test_dest_target1_part13 PARTITION OF test_dest_target1_part1 FOR VALUES IN ('c');

CREATE TABLE test_dest_target1_part2 PARTITION OF test_dest_target1
  FOR VALUES FROM (0) TO (100) PARTITION BY list (name);


CREATE TABLE test_dest_target1_part21 PARTITION OF test_dest_target1_part2 FOR VALUES IN ('a');
CREATE TABLE test_dest_target1_part22 PARTITION OF test_dest_target1_part2 FOR VALUES IN ('b');
CREATE TABLE test_dest_target1_part23 PARTITION OF test_dest_target1_part2 FOR VALUES IN ('c');

CREATE TABLE test_dest_target1_part3 PARTITION OF test_dest_target1
  FOR VALUES FROM (100) TO (200) partition BY list(name);
CREATE TABLE test_dest_target1_part31 PARTITION OF test_dest_target1_part3 FOR VALUES IN ('a');
CREATE TABLE test_dest_target1_part32 PARTITION OF test_dest_target1_part3 FOR VALUES IN ('b');
CREATE TABLE test_dest_target1_part33 PARTITION OF test_dest_target1_part3 FOR VALUES IN ('c');

CREATE TABLE test_dest_target1_part4 PARTITION OF test_dest_target1
  FOR VALUES FROM (200) TO (maxvalue) PARTITION BY list (name);

CREATE TABLE test_dest_target1_part41 PARTITION OF test_dest_target1_part4 FOR VALUES IN ('a');
CREATE TABLE test_dest_target1_part42 PARTITION OF test_dest_target1_part4 FOR VALUES IN ('b');
CREATE TABLE test_dest_target1_part43 PARTITION OF test_dest_target1_part4 FOR VALUES IN ('c');

--create index
CREATE INDEX test_dest_target1_index ON test_dest_target1(id);

--test_dest_target2
CREATE TABLE test_dest_target2(id integer,name varchar2(256)) PARTITION BY range(id);

CREATE TABLE test_dest_target2_part1 PARTITION OF test_dest_target2
  FOR VALUES FROM (minvalue) TO (0) PARTITION BY list (name);

CREATE TABLE test_dest_target2_part11 PARTITION OF test_dest_target2_part1 FOR VALUES IN ('a');
CREATE TABLE test_dest_target2_part12 PARTITION OF test_dest_target2_part1 FOR VALUES IN ('b');
CREATE TABLE test_dest_target2_part13 PARTITION OF test_dest_target2_part1 FOR VALUES IN ('c');

CREATE TABLE test_dest_target2_part2 PARTITION OF test_dest_target2
  FOR VALUES FROM (0) TO (100) PARTITION BY list (name);


CREATE TABLE test_dest_target2_part21 PARTITION OF test_dest_target2_part2 FOR VALUES IN ('a');
CREATE TABLE test_dest_target2_part22 PARTITION OF test_dest_target2_part2 FOR VALUES IN ('b');
CREATE TABLE test_dest_target2_part23 PARTITION OF test_dest_target2_part2 FOR VALUES IN ('c');

CREATE TABLE test_dest_target2_part3 PARTITION OF test_dest_target2
  FOR VALUES FROM (100) TO (200) partition BY list(name);
CREATE TABLE test_dest_target2_part31 PARTITION OF test_dest_target2_part3 FOR VALUES IN ('a');
CREATE TABLE test_dest_target2_part32 PARTITION OF test_dest_target2_part3 FOR VALUES IN ('b');
CREATE TABLE test_dest_target2_part33 PARTITION OF test_dest_target2_part3 FOR VALUES IN ('c');

CREATE TABLE test_dest_target2_part4 PARTITION OF test_dest_target2
  FOR VALUES FROM (200) TO (maxvalue) PARTITION BY list (name);

CREATE TABLE test_dest_target2_part41 PARTITION OF test_dest_target2_part4 FOR VALUES IN ('a');
CREATE TABLE test_dest_target2_part42 PARTITION OF test_dest_target2_part4 FOR VALUES IN ('b');
CREATE TABLE test_dest_target2_part43 PARTITION OF test_dest_target2_part4 FOR VALUES IN ('c');

CREATE INDEX test_dest_target2_index ON test_dest_target2(id);

--test_dest_target3
CREATE TABLE test_dest_target3(id integer,name varchar2(256)) PARTITION BY range(id);

CREATE TABLE test_dest_target3_part1 PARTITION OF test_dest_target3
  FOR VALUES FROM (minvalue) TO (0) PARTITION BY list (name);

CREATE TABLE test_dest_target3_part11 PARTITION OF test_dest_target3_part1 FOR VALUES IN ('a');
CREATE TABLE test_dest_target3_part12 PARTITION OF test_dest_target3_part1 FOR VALUES IN ('b');
CREATE TABLE test_dest_target3_part13 PARTITION OF test_dest_target3_part1 FOR VALUES IN ('c');

CREATE TABLE test_dest_target3_part2 PARTITION OF test_dest_target3
  FOR VALUES FROM (0) TO (100) PARTITION BY list (name);


CREATE TABLE test_dest_target3_part21 PARTITION OF test_dest_target3_part2 FOR VALUES IN ('a');
CREATE TABLE test_dest_target3_part22 PARTITION OF test_dest_target3_part2 FOR VALUES IN ('b');
CREATE TABLE test_dest_target3_part23 PARTITION OF test_dest_target3_part2 FOR VALUES IN ('c');

CREATE TABLE test_dest_target3_part3 PARTITION OF test_dest_target3
  FOR VALUES FROM (100) TO (200) partition BY list(name);
CREATE TABLE test_dest_target3_part31 PARTITION OF test_dest_target3_part3 FOR VALUES IN ('a');
CREATE TABLE test_dest_target3_part32 PARTITION OF test_dest_target3_part3 FOR VALUES IN ('b');
CREATE TABLE test_dest_target3_part33 PARTITION OF test_dest_target3_part3 FOR VALUES IN ('c');

CREATE TABLE test_dest_target3_part4 PARTITION OF test_dest_target3
  FOR VALUES FROM (200) TO (maxvalue) PARTITION BY list (name);

CREATE TABLE test_dest_target3_part41 PARTITION OF test_dest_target3_part4 FOR VALUES IN ('a');
CREATE TABLE test_dest_target3_part42 PARTITION OF test_dest_target3_part4 FOR VALUES IN ('b');
CREATE TABLE test_dest_target3_part43 PARTITION OF test_dest_target3_part4 FOR VALUES IN ('c');

CREATE INDEX test_dest_target3_index ON test_dest_target3(id);

INSERT INTO test_when_source VALUES(-21,'a');
INSERT INTO test_when_source VALUES(-31,'b');
INSERT INTO test_when_source VALUES(-41,'c');

INSERT INTO test_when_source VALUES(2,'a');
INSERT INTO test_when_source VALUES(25,'b');
INSERT INTO test_when_source VALUES(95,'c');

INSERT INTO test_when_source VALUES(100,'c');
INSERT INTO test_when_source VALUES(120,'b');
INSERT INTO test_when_source VALUES(150,'a');

INSERT INTO test_when_source VALUES(300,'c');
INSERT INTO test_when_source VALUES(420,'b');
INSERT INTO test_when_source VALUES(550,'a');


--all has policy
ALTER TABLE test_when_source enable ROW LEVEL SECURITY;

ALTER TABLE test_when_source_part1 enable ROW LEVEL SECURITY;
ALTER TABLE test_when_source_part11 enable ROW LEVEL SECURITY;
ALTER TABLE test_when_source_part12 enable ROW LEVEL SECURITY;
ALTER TABLE test_when_source_part13 enable ROW LEVEL SECURITY;

ALTER TABLE test_when_source_part2 enable ROW LEVEL SECURITY;
ALTER TABLE test_when_source_part21 enable ROW LEVEL SECURITY;
ALTER TABLE test_when_source_part22 enable ROW LEVEL SECURITY;
ALTER TABLE test_when_source_part23 enable ROW LEVEL SECURITY;

ALTER TABLE test_when_source_part3 enable ROW LEVEL SECURITY;
ALTER TABLE test_when_source_part31 enable ROW LEVEL SECURITY;
ALTER TABLE test_when_source_part32 enable ROW LEVEL SECURITY;
ALTER TABLE test_when_source_part33 enable ROW LEVEL SECURITY;

ALTER TABLE test_when_source_part4 enable ROW LEVEL SECURITY;
ALTER TABLE test_when_source_part41 enable ROW LEVEL SECURITY;
ALTER TABLE test_when_source_part42 enable ROW LEVEL SECURITY;
ALTER TABLE test_when_source_part43 enable ROW LEVEL SECURITY;

ALTER TABLE test_dest_target1 enable ROW LEVEL SECURITY;

ALTER TABLE test_dest_target1_part1 enable ROW LEVEL SECURITY;
ALTER TABLE test_dest_target1_part11 enable ROW LEVEL SECURITY;
ALTER TABLE test_dest_target1_part12 enable ROW LEVEL SECURITY;
ALTER TABLE test_dest_target1_part13 enable ROW LEVEL SECURITY;

ALTER TABLE test_dest_target1_part2 enable ROW LEVEL SECURITY;
ALTER TABLE test_dest_target1_part21 enable ROW LEVEL SECURITY;
ALTER TABLE test_dest_target1_part22 enable ROW LEVEL SECURITY;
ALTER TABLE test_dest_target1_part23 enable ROW LEVEL SECURITY;

ALTER TABLE test_dest_target1_part3 enable ROW LEVEL SECURITY;
ALTER TABLE test_dest_target1_part31 enable ROW LEVEL SECURITY;
ALTER TABLE test_dest_target1_part32 enable ROW LEVEL SECURITY;
ALTER TABLE test_dest_target1_part33 enable ROW LEVEL SECURITY;

ALTER TABLE test_dest_target1_part4 enable ROW LEVEL SECURITY;
ALTER TABLE test_dest_target1_part41 enable ROW LEVEL SECURITY;
ALTER TABLE test_dest_target1_part42 enable ROW LEVEL SECURITY;
ALTER TABLE test_dest_target1_part43 enable ROW LEVEL SECURITY;


ALTER TABLE test_dest_target2 enable ROW LEVEL SECURITY;

ALTER TABLE test_dest_target2_part1 enable ROW LEVEL SECURITY;
ALTER TABLE test_dest_target2_part11 enable ROW LEVEL SECURITY;
ALTER TABLE test_dest_target2_part12 enable ROW LEVEL SECURITY;
ALTER TABLE test_dest_target2_part13 enable ROW LEVEL SECURITY;

ALTER TABLE test_dest_target2_part2 enable ROW LEVEL SECURITY;
ALTER TABLE test_dest_target2_part21 enable ROW LEVEL SECURITY;
ALTER TABLE test_dest_target2_part22 enable ROW LEVEL SECURITY;
ALTER TABLE test_dest_target2_part23 enable ROW LEVEL SECURITY;

ALTER TABLE test_dest_target2_part3 enable ROW LEVEL SECURITY;
ALTER TABLE test_dest_target2_part31 enable ROW LEVEL SECURITY;
ALTER TABLE test_dest_target2_part32 enable ROW LEVEL SECURITY;
ALTER TABLE test_dest_target2_part33 enable ROW LEVEL SECURITY;

ALTER TABLE test_dest_target2_part4 enable ROW LEVEL SECURITY;
ALTER TABLE test_dest_target2_part41 enable ROW LEVEL SECURITY;
ALTER TABLE test_dest_target2_part42 enable ROW LEVEL SECURITY;
ALTER TABLE test_dest_target2_part43 enable ROW LEVEL SECURITY;

ALTER TABLE test_dest_target3 enable ROW LEVEL SECURITY;

ALTER TABLE test_dest_target3_part1 enable ROW LEVEL SECURITY;
ALTER TABLE test_dest_target3_part11 enable ROW LEVEL SECURITY;
ALTER TABLE test_dest_target3_part12 enable ROW LEVEL SECURITY;
ALTER TABLE test_dest_target3_part13 enable ROW LEVEL SECURITY;

ALTER TABLE test_dest_target3_part2 enable ROW LEVEL SECURITY;
ALTER TABLE test_dest_target3_part21 enable ROW LEVEL SECURITY;
ALTER TABLE test_dest_target3_part22 enable ROW LEVEL SECURITY;
ALTER TABLE test_dest_target3_part23 enable ROW LEVEL SECURITY;

ALTER TABLE test_dest_target3_part3 enable ROW LEVEL SECURITY;
ALTER TABLE test_dest_target3_part31 enable ROW LEVEL SECURITY;
ALTER TABLE test_dest_target3_part32 enable ROW LEVEL SECURITY;
ALTER TABLE test_dest_target3_part33 enable ROW LEVEL SECURITY;

ALTER TABLE test_dest_target3_part4 enable ROW LEVEL SECURITY;
ALTER TABLE test_dest_target3_part41 enable ROW LEVEL SECURITY;
ALTER TABLE test_dest_target3_part42 enable ROW LEVEL SECURITY;
ALTER TABLE test_dest_target3_part43 enable ROW LEVEL SECURITY;

--enable force
ALTER TABLE test_when_source force ROW LEVEL SECURITY;

ALTER TABLE test_when_source_part1 force ROW LEVEL SECURITY;
ALTER TABLE test_when_source_part11 force ROW LEVEL SECURITY;
ALTER TABLE test_when_source_part12 force ROW LEVEL SECURITY;
ALTER TABLE test_when_source_part13 force ROW LEVEL SECURITY;

ALTER TABLE test_when_source_part2 force ROW LEVEL SECURITY;
ALTER TABLE test_when_source_part21 force ROW LEVEL SECURITY;
ALTER TABLE test_when_source_part22 force ROW LEVEL SECURITY;
ALTER TABLE test_when_source_part23 force ROW LEVEL SECURITY;

ALTER TABLE test_when_source_part3 force ROW LEVEL SECURITY;
ALTER TABLE test_when_source_part31 force ROW LEVEL SECURITY;
ALTER TABLE test_when_source_part32 force ROW LEVEL SECURITY;
ALTER TABLE test_when_source_part33 force ROW LEVEL SECURITY;

ALTER TABLE test_when_source_part4 force ROW LEVEL SECURITY;
ALTER TABLE test_when_source_part41 force ROW LEVEL SECURITY;
ALTER TABLE test_when_source_part42 force ROW LEVEL SECURITY;
ALTER TABLE test_when_source_part43 force ROW LEVEL SECURITY;


ALTER TABLE test_dest_target1 force ROW LEVEL SECURITY;

ALTER TABLE test_dest_target1_part1 force ROW LEVEL SECURITY;
ALTER TABLE test_dest_target1_part11 force ROW LEVEL SECURITY;
ALTER TABLE test_dest_target1_part12 force ROW LEVEL SECURITY;
ALTER TABLE test_dest_target1_part13 force ROW LEVEL SECURITY;

ALTER TABLE test_dest_target1_part2 force ROW LEVEL SECURITY;
ALTER TABLE test_dest_target1_part21 force ROW LEVEL SECURITY;
ALTER TABLE test_dest_target1_part22 force ROW LEVEL SECURITY;
ALTER TABLE test_dest_target1_part23 force ROW LEVEL SECURITY;

ALTER TABLE test_dest_target1_part3 force ROW LEVEL SECURITY;
ALTER TABLE test_dest_target1_part31 force ROW LEVEL SECURITY;
ALTER TABLE test_dest_target1_part32 force ROW LEVEL SECURITY;
ALTER TABLE test_dest_target1_part33 force ROW LEVEL SECURITY;

ALTER TABLE test_dest_target1_part4 force ROW LEVEL SECURITY;
ALTER TABLE test_dest_target1_part41 force ROW LEVEL SECURITY;
ALTER TABLE test_dest_target1_part42 force ROW LEVEL SECURITY;
ALTER TABLE test_dest_target1_part43 force ROW LEVEL SECURITY;


ALTER TABLE test_dest_target2 force ROW LEVEL SECURITY;

ALTER TABLE test_dest_target2_part1 force ROW LEVEL SECURITY;
ALTER TABLE test_dest_target2_part11 force ROW LEVEL SECURITY;
ALTER TABLE test_dest_target2_part12 force ROW LEVEL SECURITY;
ALTER TABLE test_dest_target2_part13 force ROW LEVEL SECURITY;

ALTER TABLE test_dest_target2_part2 force ROW LEVEL SECURITY;
ALTER TABLE test_dest_target2_part21 force ROW LEVEL SECURITY;
ALTER TABLE test_dest_target2_part22 force ROW LEVEL SECURITY;
ALTER TABLE test_dest_target2_part23 force ROW LEVEL SECURITY;

ALTER TABLE test_dest_target2_part3 force ROW LEVEL SECURITY;
ALTER TABLE test_dest_target2_part31 force ROW LEVEL SECURITY;
ALTER TABLE test_dest_target2_part32 force ROW LEVEL SECURITY;
ALTER TABLE test_dest_target2_part33 force ROW LEVEL SECURITY;

ALTER TABLE test_dest_target2_part4 force ROW LEVEL SECURITY;
ALTER TABLE test_dest_target2_part41 force ROW LEVEL SECURITY;
ALTER TABLE test_dest_target2_part42 force ROW LEVEL SECURITY;
ALTER TABLE test_dest_target2_part43 force ROW LEVEL SECURITY;

ALTER TABLE test_dest_target3 force ROW LEVEL SECURITY;

ALTER TABLE test_dest_target3_part1 force ROW LEVEL SECURITY;
ALTER TABLE test_dest_target3_part11 force ROW LEVEL SECURITY;
ALTER TABLE test_dest_target3_part12 force ROW LEVEL SECURITY;
ALTER TABLE test_dest_target3_part13 force ROW LEVEL SECURITY;

ALTER TABLE test_dest_target3_part2 force ROW LEVEL SECURITY;
ALTER TABLE test_dest_target3_part21 force ROW LEVEL SECURITY;
ALTER TABLE test_dest_target3_part22 force ROW LEVEL SECURITY;
ALTER TABLE test_dest_target3_part23 force ROW LEVEL SECURITY;

ALTER TABLE test_dest_target3_part3 force ROW LEVEL SECURITY;
ALTER TABLE test_dest_target3_part31 force ROW LEVEL SECURITY;
ALTER TABLE test_dest_target3_part32 force ROW LEVEL SECURITY;
ALTER TABLE test_dest_target3_part33 force ROW LEVEL SECURITY;

ALTER TABLE test_dest_target3_part4 force ROW LEVEL SECURITY;
ALTER TABLE test_dest_target3_part41 force ROW LEVEL SECURITY;
ALTER TABLE test_dest_target3_part42 force ROW LEVEL SECURITY;
ALTER TABLE test_dest_target3_part43 force ROW LEVEL SECURITY;

--add policy
CREATE policy test_source_policy1 ON test_when_source
FOR SELECT TO CURRENT_USER
using (true);

CREATE TABLE test_when_source1 AS SELECT * FROM test_when_source;

CREATE policy test_source_policy ON test_when_source
FOR SELECT TO CURRENT_USER
using (id IN (SELECT id FROM test_when_source1));

--test with check
CREATE policy test_target1_policy ON test_dest_target1 FOR INSERT WITH CHECK
(id < 10000);
CREATE policy test_target1_policy ON test_dest_target1_part1 FOR INSERT WITH CHECK
(id < 10000);
CREATE policy test_target1_policy ON test_dest_target1_part11 FOR INSERT WITH CHECK
(id < 10000);
CREATE policy test_target1_policy ON test_dest_target1_part12 FOR INSERT WITH CHECK
(id < 10000);
CREATE policy test_target1_policy ON test_dest_target1_part13 FOR INSERT WITH CHECK
(id < 10000);

CREATE policy test_target1_policy ON test_dest_target1_part2 FOR INSERT WITH CHECK
(id < 10000);
CREATE policy test_target1_policy ON test_dest_target1_part21 FOR INSERT WITH CHECK
(id < 10000);
CREATE policy test_target1_policy ON test_dest_target1_part22 FOR INSERT WITH CHECK
(id < 10000);
CREATE policy test_target1_policy ON test_dest_target1_part23 FOR INSERT WITH CHECK
(id < 10000);

CREATE policy test_target1_policy ON test_dest_target1_part3 FOR INSERT WITH CHECK
(id < 10000);
CREATE policy test_target1_policy ON test_dest_target1_part31 FOR INSERT WITH CHECK
(id < 10000);
CREATE policy test_target1_policy ON test_dest_target1_part32 FOR INSERT WITH CHECK
(id < 10000);
CREATE policy test_target1_policy ON test_dest_target1_part33 FOR INSERT WITH CHECK
(id < 10000);

CREATE policy test_target1_policy ON test_dest_target1_part4 FOR INSERT WITH CHECK
(id < 10000);
CREATE policy test_target1_policy ON test_dest_target1_part41 FOR INSERT WITH CHECK
(id < 10000);
CREATE policy test_target1_policy ON test_dest_target1_part42 FOR INSERT WITH CHECK
(id < 10000);
CREATE policy test_target1_policy ON test_dest_target1_part43 FOR INSERT WITH CHECK
(id < 10000);

CREATE policy test_target2_policy ON test_dest_target2 FOR INSERT WITH
CHECK (id IN (SELECT id FROM test_when_source));

CREATE policy test_target2_policy ON test_dest_target2_part1 FOR INSERT WITH
CHECK (id IN (SELECT id FROM test_when_source));
CREATE policy test_target2_policy ON test_dest_target2_part11 FOR INSERT WITH
CHECK (id IN (SELECT id FROM test_when_source));
CREATE policy test_target2_policy ON test_dest_target2_part12 FOR INSERT WITH
CHECK (id IN (SELECT id FROM test_when_source));
CREATE policy test_target2_policy ON test_dest_target2_part13 FOR INSERT WITH
CHECK (id IN (SELECT id FROM test_when_source));

CREATE policy test_target2_policy ON test_dest_target2_part2 FOR INSERT WITH
CHECK (id IN (SELECT id FROM test_when_source));
CREATE policy test_target2_policy ON test_dest_target2_part21 FOR INSERT WITH
CHECK (id IN (SELECT id FROM test_when_source));
CREATE policy test_target2_policy ON test_dest_target2_part22 FOR INSERT WITH
CHECK (id IN (SELECT id FROM test_when_source));
CREATE policy test_target2_policy ON test_dest_target2_part23 FOR INSERT WITH
CHECK (id IN (SELECT id FROM test_when_source));


CREATE policy test_target2_policy ON test_dest_target2_part3 FOR INSERT WITH
CHECK (id IN (SELECT id FROM test_when_source));
CREATE policy test_target2_policy ON test_dest_target2_part31 FOR INSERT WITH
CHECK (id IN (SELECT id FROM test_when_source));
CREATE policy test_target2_policy ON test_dest_target2_part32 FOR INSERT WITH
CHECK (id IN (SELECT id FROM test_when_source));
CREATE policy test_target2_policy ON test_dest_target2_part33 FOR INSERT WITH
CHECK (id IN (SELECT id FROM test_when_source));


CREATE policy test_target2_policy ON test_dest_target2_part4 FOR INSERT WITH
CHECK (id IN (SELECT id FROM test_when_source));
CREATE policy test_target2_policy ON test_dest_target2_part41 FOR INSERT WITH
CHECK (id IN (SELECT id FROM test_when_source));
CREATE policy test_target2_policy ON test_dest_target2_part42 FOR INSERT WITH
CHECK (id IN (SELECT id FROM test_when_source));
CREATE policy test_target2_policy ON test_dest_target2_part43 FOR INSERT WITH
CHECK (id IN (SELECT id FROM test_when_source));

CREATE policy test_target3_policy ON test_dest_target3 FOR INSERT WITH
CHECK (id IN (SELECT id FROM test_when_source1));

CREATE policy test_target3_policy ON test_dest_target3_part1 FOR INSERT WITH
CHECK (id IN (SELECT id FROM test_when_source1));
CREATE policy test_target3_policy ON test_dest_target3_part11 FOR INSERT WITH
CHECK (id IN (SELECT id FROM test_when_source1));
CREATE policy test_target3_policy ON test_dest_target3_part12 FOR INSERT WITH
CHECK (id IN (SELECT id FROM test_when_source1));
CREATE policy test_target3_policy ON test_dest_target3_part13 FOR INSERT WITH
CHECK (id IN (SELECT id FROM test_when_source1));

CREATE policy test_target3_policy ON test_dest_target3_part2 FOR INSERT WITH
CHECK (id IN (SELECT id FROM test_when_source1));
CREATE policy test_target3_policy ON test_dest_target3_part21 FOR INSERT WITH
CHECK (id IN (SELECT id FROM test_when_source1));
CREATE policy test_target3_policy ON test_dest_target3_part22 FOR INSERT WITH
CHECK (id IN (SELECT id FROM test_when_source1));
CREATE policy test_target3_policy ON test_dest_target3_part23 FOR INSERT WITH
CHECK (id IN (SELECT id FROM test_when_source1));

CREATE policy test_target3_policy ON test_dest_target3_part3 FOR INSERT WITH
CHECK (id IN (SELECT id FROM test_when_source1));
CREATE policy test_target3_policy ON test_dest_target3_part31 FOR INSERT WITH
CHECK (id IN (SELECT id FROM test_when_source1));
CREATE policy test_target3_policy ON test_dest_target3_part32 FOR INSERT WITH
CHECK (id IN (SELECT id FROM test_when_source1));
CREATE policy test_target3_policy ON test_dest_target3_part33 FOR INSERT WITH
CHECK (id IN (SELECT id FROM test_when_source1));


CREATE policy test_target3_policy ON test_dest_target3_part4 FOR INSERT WITH
CHECK (id IN (SELECT id FROM test_when_source1));
CREATE policy test_target3_policy ON test_dest_target3_part41 FOR INSERT WITH
CHECK (id IN (SELECT id FROM test_when_source1));
CREATE policy test_target3_policy ON test_dest_target3_part42 FOR INSERT WITH
CHECK (id IN (SELECT id FROM test_when_source1));
CREATE policy test_target3_policy ON test_dest_target3_part43 FOR INSERT WITH
CHECK (id IN (SELECT id FROM test_when_source1));

CREATE policy test_target1_policy1 ON test_dest_target1
FOR SELECT TO CURRENT_USER
using (id < 1000);

CREATE policy test_target2_policy1 ON test_dest_target2
FOR SELECT TO CURRENT_USER
using (id < 1000);

CREATE policy test_target3_policy1 ON test_dest_target3
FOR SELECT TO CURRENT_USER
using (id < 1000);

--36 rows
INSERT all
     INTO test_dest_target1
     INTO test_dest_target3
     INTO test_dest_target2
SELECT  *FROM test_when_source ORDER BY id;

--12 rows
SELECT * FROM test_dest_target1 ORDER BY id;
--12 rows
SELECT * FROM test_dest_target2 ORDER BY id;
--12 rows
SELECT * FROM test_dest_target3 ORDER BY id;

--12 rows
INSERT first
   WHEN id < 2 then
     INTO test_dest_target1
   WHEN id < 300 then
     INTO test_dest_target3
   else
     INTO test_dest_target2
SELECT  *FROM test_when_source ORDER BY id;

--12 rows
INSERT first
   WHEN id < 2 then
     INTO test_dest_target1
   WHEN id < 3 then
     INTO test_dest_target3
   else
     INTO test_dest_target2
SELECT  *FROM test_when_source ORDER BY id;

--18 rows
SELECT * FROM test_dest_target1 ORDER BY id;
--23 rows
SELECT * FROM test_dest_target2 ORDER BY id;
--19 rows
SELECT * FROM test_dest_target3 ORDER BY id;

SET row_security = on;


INSERT first
   WHEN id < 2 then
     INTO test_dest_target1
	 INTO test_dest_target1
	 INTO test_dest_target1
   WHEN id < 3 then
     INTO test_dest_target3
	 INTO test_dest_target3
	 INTO test_dest_target3
   else
     INTO test_dest_target2
	 INTO test_dest_target2
SELECT  *FROM test_when_source ORDER BY id;

--27 rows
SELECT * FROM test_dest_target1 ORDER BY id;
--39 rows
SELECT * FROM test_dest_target2 ORDER BY id;
--22 rows
SELECT * FROM test_dest_target3 ORDER BY id;

prepare test_mulit_insert (integer,varchar2) AS
INSERT ALL
   WHEN id < 3 then
     INTO test_dest_target1 values($1,$2)
	 INTO test_dest_target1 VALUES($1,$2)
	 into test_dest_target1
   else
     INTO test_dest_target3
     INTO test_dest_target2
SELECT  *FROM test_when_source UNION
ALL
  SELECT $1,$2;

\set EXECUTE_RUN_PREPARE ON

--ok
EXECUTE test_mulit_insert(1,'a');
EXECUTE test_mulit_insert(2,'b');

DEALLOCATE test_mulit_insert;

--add with
--12 rows
WITH a AS (SELECT * FROM test_when_source ORDER BY id)
INSERT first
   WHEN id < 2 then
     INTO test_dest_target1
   WHEN id < 3 then
     INTO test_dest_target3
   else
     INTO test_dest_target2
SELECT  *FROM a;

---all test_target add another policy
CREATE OR REPLACE FUNCTION test_f(id integer) RETURN integer IS
BEGIN
  raise info 'test_f id:%', id;
  RETURN id;
end;
/

CREATE policy test_target1_policy3 ON test_dest_target1 AS RESTRICTIVE FOR INSERT WITH CHECK
(id < 2000);
CREATE policy test_target1_policy3 ON test_dest_target1_part1 AS RESTRICTIVE FOR INSERT WITH CHECK
(id < 2000);
CREATE policy test_target1_policy3 ON test_dest_target1_part11 AS RESTRICTIVE FOR INSERT WITH CHECK
(id < 2000);
CREATE policy test_target1_policy3 ON test_dest_target1_part12 AS RESTRICTIVE FOR INSERT WITH CHECK
(id < 2000);
CREATE policy test_target1_policy3 ON test_dest_target1_part13 AS RESTRICTIVE FOR INSERT WITH CHECK
(id < 2000);

CREATE policy test_target1_policy3 ON test_dest_target1_part2 AS RESTRICTIVE FOR INSERT WITH CHECK
(id < 2000);
CREATE policy test_target1_policy3 ON test_dest_target1_part21 AS RESTRICTIVE FOR INSERT WITH CHECK
(id < 2000);
CREATE policy test_target1_policy3 ON test_dest_target1_part22 AS RESTRICTIVE FOR INSERT WITH CHECK
(id < 2000);
CREATE policy test_target1_policy3 ON test_dest_target1_part23 AS RESTRICTIVE FOR INSERT WITH CHECK
(id < 2000);

CREATE policy test_target1_policy3 ON test_dest_target1_part3 AS RESTRICTIVE FOR INSERT WITH CHECK
(id < 2000);
CREATE policy test_target1_policy3 ON test_dest_target1_part31 AS RESTRICTIVE FOR INSERT WITH CHECK
(id < 2000);
CREATE policy test_target1_policy3 ON test_dest_target1_part32 AS RESTRICTIVE FOR INSERT WITH CHECK
(id < 2000);
CREATE policy test_target1_policy3 ON test_dest_target1_part33 AS RESTRICTIVE FOR INSERT WITH CHECK
(id < 2000);

CREATE policy test_target1_policy3 ON test_dest_target1_part4 AS RESTRICTIVE FOR INSERT WITH CHECK
(id < 2000);
CREATE policy test_target1_policy3 ON test_dest_target1_part41 AS RESTRICTIVE FOR INSERT WITH CHECK
(id < 2000);
CREATE policy test_target1_policy3 ON test_dest_target1_part42 AS RESTRICTIVE FOR INSERT WITH CHECK
(id < 2000);
CREATE policy test_target1_policy3 ON test_dest_target1_part43 AS RESTRICTIVE FOR INSERT WITH CHECK
(id < 2000);

CREATE policy test_target2_policy3 ON test_dest_target2 FOR INSERT WITH
CHECK (id NOT IN (SELECT id FROM test_when_source));

CREATE policy test_target2_policy3 ON test_dest_target2_part1 FOR INSERT WITH
CHECK (id NOT IN (SELECT id FROM test_when_source));
CREATE policy test_target2_policy3 ON test_dest_target2_part11 FOR INSERT WITH
CHECK (id NOT IN (SELECT id FROM test_when_source));
CREATE policy test_target2_policy3 ON test_dest_target2_part12 FOR INSERT WITH
CHECK (id NOT IN (SELECT id FROM test_when_source));
CREATE policy test_target2_policy3 ON test_dest_target2_part13 FOR INSERT WITH
CHECK (id NOT IN (SELECT id FROM test_when_source));

CREATE policy test_target2_policy3 ON test_dest_target2_part2 FOR INSERT WITH
CHECK (id NOT IN (SELECT id FROM test_when_source));
CREATE policy test_target2_policy3 ON test_dest_target2_part21 FOR INSERT WITH
CHECK (id NOT IN (SELECT id FROM test_when_source));
CREATE policy test_target2_policy3 ON test_dest_target2_part22 FOR INSERT WITH
CHECK (id NOT IN (SELECT id FROM test_when_source));
CREATE policy test_target2_policy3 ON test_dest_target2_part23 FOR INSERT WITH
CHECK (id NOT IN (SELECT id FROM test_when_source));

CREATE policy test_target2_policy3 ON test_dest_target2_part3 FOR INSERT WITH
CHECK (id NOT IN (SELECT id FROM test_when_source));
CREATE policy test_target2_policy3 ON test_dest_target2_part31 FOR INSERT WITH
CHECK (id NOT IN (SELECT id FROM test_when_source));
CREATE policy test_target2_policy3 ON test_dest_target2_part32 FOR INSERT WITH
CHECK (id NOT IN (SELECT id FROM test_when_source));
CREATE policy test_target2_policy3 ON test_dest_target2_part33 FOR INSERT WITH
CHECK (id NOT IN (SELECT id FROM test_when_source));


CREATE policy test_target2_policy3 ON test_dest_target2_part4 FOR INSERT WITH
CHECK (id NOT IN (SELECT id FROM test_when_source));
CREATE policy test_target2_policy3 ON test_dest_target2_part41 FOR INSERT WITH
CHECK (id NOT IN (SELECT id FROM test_when_source));
CREATE policy test_target2_policy3 ON test_dest_target2_part42 FOR INSERT WITH
CHECK (id NOT IN (SELECT id FROM test_when_source));
CREATE policy test_target2_policy3 ON test_dest_target2_part43 FOR INSERT WITH
CHECK (id NOT IN (SELECT id FROM test_when_source));

CREATE policy test_target3_policy3 ON test_dest_target3 AS RESTRICTIVE FOR INSERT WITH
CHECK (id  = test_f(id));

CREATE policy test_target3_policy3 ON test_dest_target3_part1 AS RESTRICTIVE FOR INSERT WITH
CHECK (id  = test_f(id));
CREATE policy test_target3_policy3 ON test_dest_target3_part11 AS RESTRICTIVE FOR INSERT WITH
CHECK (id  = test_f(id));
CREATE policy test_target3_policy3 ON test_dest_target3_part12 AS RESTRICTIVE FOR INSERT WITH
CHECK (id  = test_f(id));
CREATE policy test_target3_policy3 ON test_dest_target3_part13 AS RESTRICTIVE FOR INSERT WITH
CHECK (id  = test_f(id));

CREATE policy test_target3_policy3 ON test_dest_target3_part2 AS RESTRICTIVE FOR INSERT WITH
CHECK (id  = test_f(id));
CREATE policy test_target3_policy3 ON test_dest_target3_part21 AS RESTRICTIVE FOR INSERT WITH
CHECK (id  = test_f(id));
CREATE policy test_target3_policy3 ON test_dest_target3_part22 AS RESTRICTIVE FOR INSERT WITH
CHECK (id  = test_f(id));
CREATE policy test_target3_policy3 ON test_dest_target3_part23 AS RESTRICTIVE FOR INSERT WITH
CHECK (id  = test_f(id));


CREATE policy test_target3_policy3 ON test_dest_target3_part3 AS RESTRICTIVE FOR INSERT WITH
CHECK (id  = test_f(id));
CREATE policy test_target3_policy3 ON test_dest_target3_part31 AS RESTRICTIVE FOR INSERT WITH
CHECK (id  = test_f(id));
CREATE policy test_target3_policy3 ON test_dest_target3_part32 AS RESTRICTIVE FOR INSERT WITH
CHECK (id  = test_f(id));
CREATE policy test_target3_policy3 ON test_dest_target3_part33 AS RESTRICTIVE FOR INSERT WITH
CHECK (id  = test_f(id));


CREATE policy test_target3_policy3 ON test_dest_target3_part4 AS RESTRICTIVE FOR INSERT WITH
CHECK (id  = test_f(id));
CREATE policy test_target3_policy3 ON test_dest_target3_part41 AS RESTRICTIVE FOR INSERT WITH
CHECK (id  = test_f(id));
CREATE policy test_target3_policy3 ON test_dest_target3_part42 AS RESTRICTIVE FOR INSERT WITH
CHECK (id  = test_f(id));
CREATE policy test_target3_policy3 ON test_dest_target3_part43 AS RESTRICTIVE FOR INSERT WITH
CHECK (id  = test_f(id));

--21 rows
WITH a AS (SELECT * FROM test_when_source ORDER BY id)
INSERT first
   WHEN id < 2 then
     INTO test_dest_target1
   else
     INTO test_dest_target3
     INTO test_dest_target2
SELECT  *FROM a;

--test test_when_source is a view
CREATE VIEW test_view AS SELECT * FROM test_when_source;

--21 rows
INSERT first
   WHEN id < 2 then
     INTO test_dest_target1 VALUES(id,name)
   else
     INTO test_dest_target3 VALUES(id,name)
     INTO test_dest_target2 VALUES(id,name)
SELECT  name,id FROM test_view ORDER BY id;

--clean data
DROP VIEW test_view;
DROP TABLE test_dest_target1;
DROP TABLE test_dest_target3;
DROP TABLE test_dest_target2;
DROP FUNCTION test_f;
DROP TABLE test_when_source;
DROP TABLE test_when_source1;
RESET SESSION AUTHORIZATION;
DROP schema test_insert_user;
DROP USER test_insert_user;

set ivorysql.default_with_rowids = on;

create table parted_trig (a int) partition by list (a);

create table parted_trig2 partition of parted_trig for values in (2);

create table parted_trig3 (like parted_trig);

alter table parted_trig attach partition parted_trig3 for values in (3);

create table source_table(id integer);

insert into source_table values(2), (3);
insert into source_table values(2), (3);

--ok
insert ALL
  when id = 2 then
  into parted_trig2
  when id = 3 then
   into parted_trig3
  when id > 1 then
    into parted_trig
select * from source_table;

select * from parted_trig order by a;
select * from parted_trig2 order by a;
select * from parted_trig3 order by a;


--test errlog_log grammer
insert ALL
  into parted_trig LOG ERRORS INTO test
  into parted_trig LOG ERRORS INTO tetst.test(b)
  into parted_trig LOG ERRORS REJECT LIMIT 2
  into parted_trig LOG ERRORS REJECT LIMIT UNLIMITED
  into parted_trig LOG ERRORS INTO schema.test(ab) REJECT LIMIT 2
select  *from parted_trig;

select * from parted_trig order by a;
select * from parted_trig2 order by a;
select * from parted_trig3 order by a;


insert first
  when id > 1 then
	  into parted_trig LOG ERRORS INTO test
	  into parted_trig LOG ERRORS INTO test.test(ab)
  when id > 2 then
	  into parted_trig LOG ERRORS REJECT LIMIT 2
	  into parted_trig LOG ERRORS REJECT LIMIT UNLIMITED
  else
	into parted_trig LOG ERRORS INTO test.test(ab) REJECT LIMIT 2
select  *from source_table;

select * from parted_trig order by a;
select * from parted_trig2 order by a;
select * from parted_trig3 order by a;

--clean data
drop table parted_trig;
drop table source_table;


CREATE TABLE vtest1_part (a number, b number GENERATED ALWAYS AS (a * 2), name varchar2(256), id number) partition by range(a);

--part1 level
CREATE TABLE vtest1_part_part1 PARTITION OF vtest1_part
  FOR VALUES FROM (minvalue) TO (0) PARTITION BY range (id);

create table vtest1_part_part11 partition of vtest1_part_part1
  for values from (minvalue) to (0);
create table vtest1_part_part12 partition of vtest1_part_part1
  for values from (0) to (2000);
create table vtest1_part_part13 partition of vtest1_part_part1
  for values from (2000) to (maxvalue);

create table vtest1_part_part2 partition of vtest1_part
 for values from (0) to (200) partition by list(name);

create table vtest1_part_part21 partition of vtest1_part_part2
 for values in ('name1');
create table vtest1_part_part22 partition of vtest1_part_part2
 for values in ('name2');
create table vtest1_part_part23 partition of vtest1_part_part2
 for values in ('name3');

create table vtest1_part_part3 partition of vtest1_part
   for values from (200) to (1000) partition by range(id);
create table vtest1_part_part31 partition of vtest1_part_part3
  for values from (minvalue) to (0);
create table vtest1_part_part32 partition of vtest1_part_part3
  for values from (0) to (2000);
create table vtest1_part_part33 partition of vtest1_part_part3
  for values from (2000) to (maxvalue);

create table vtest1_part_part4 partition of vtest1_part
  for values from (1000) to (maxvalue) partition by range(id);
create table vtest1_part_part41 partition of vtest1_part_part4
  for values from (minvalue) to (0);
create table vtest1_part_part42 partition of vtest1_part_part4
  for values from (0) to (2000);
create table vtest1_part_part43 partition of vtest1_part_part4
  for values from (2000) to (maxvalue);


CREATE TABLE tab1_part (name varchar2(256), id NUMBER,description VARCHAR2(50) INVISIBLE, a number) partition by range(id);

CREATE TABLE tab1_part_part1 PARTITION OF tab1_part
  FOR VALUES FROM (minvalue) TO (0) PARTITION BY range (a);

create table tab1_part_part11 partition of tab1_part_part1
  for values from (minvalue) to (0);
create table tab1_part_part12 partition of tab1_part_part1
  for values from (0) to (2000);
create table tab1_part_part13 partition of tab1_part_part1
  for values from (2000) to (maxvalue);

create table tab1_part_part2 partition of tab1_part
 for values from (0) to (200) partition by list(name);

create table tab1_part_part21 partition of tab1_part_part2
 for values in ('name1');
create table tab1_part_part22 partition of tab1_part_part2
 for values in ('name2');
create table tab1_part_part23 partition of tab1_part_part2
 for values in ('name3');

create table tab1_part_part3 partition of tab1_part
   for values from (200) to (1000) partition by range(a);
create table tab1_part_part31 partition of tab1_part_part3
  for values from (minvalue) to (0);
create table tab1_part_part32 partition of tab1_part_part3
  for values from (0) to (2000);
create table tab1_part_part33 partition of tab1_part_part3
  for values from (2000) to (maxvalue);

create table tab1_part_part4 partition of tab1_part
  for values from (1000) to (maxvalue) partition by range(a);
create table tab1_part_part41 partition of tab1_part_part4
  for values from (minvalue) to (0);
create table tab1_part_part42 partition of tab1_part_part4
  for values from (0) to (2000);
create table tab1_part_part43 partition of tab1_part_part4
  for values from (2000) to (maxvalue);

CREATE TABLE test_source(id number,name varchar2(256),a number);

insert into test_source values(-1,'name1',-1);
insert into test_source values(-56,'name2',-25);
insert into test_source values(-156,'name3',-156);

insert into test_source values(1,'name1',1);
insert into test_source values(56,'name2',25);
insert into test_source values(156,'name3',156);

insert into test_source values(256,'name1',256);
insert into test_source values(356,'name2',356);
insert into test_source values(456,'name3',456);



insert into test_source values(1256,'name1',1256);
insert into test_source values(1356,'name2',1356);
insert into test_source values(1456,'name3',1456);


insert into test_source values(2256,'name1',2256);
insert into test_source values(2356,'name2',2356);
insert into test_source values(2456,'name3',2456);


insert into test_source values(4256,'name1',4256);
insert into test_source values(4356,'name2',4356);
insert into test_source values(4456,'name3',4456);

insert into test_source values(201,'name1', -201);
insert into test_source values(845,'name2', -845);
insert into test_source values(945,'name3', -95);
insert into test_source values(2001,'name3', -2001);


insert into test_source values(-1,'name1', 201);
insert into test_source values(845,'name2', 845);
insert into test_source values(2001,'name3', 948);

insert into test_source values(-4,'name1', 2001);
insert into test_source values(201,'name2', 2004);


--error
insert all
   when id > 1000 and a > 2000 then
		into tab1_part_part43(a,name,id) values(a,name,id)
   when id > 1000 and a > 0 then
		into tab1_part_part42(a,name,id) values(a,name,id)
   when id > 1000 and a < 0 then
		into tab1_part_part41(a,name,id) values(a,name,id)
   when id > 1000 then
		into tab1_part_part4(a,name,id) values(a,name,id)
   --
   when id > 200 and id < 1000 and a > 2000 then
	into tab1_part_part33(a,name,id) values(a,name,id)
   when id > 200 and id < 1000 and a > 0 then
	into tab1_part_part32(a,name,id) values(a,name,id)
   when id > 200 and id < 1000 and a < 0 then
	into tab1_part_part31(a,name,id) values(a,name,id)
   when id > 200 and id < 1000 then
	into tab1_part_part3(a,name,id) values(a,name,id)

   ---
   when id >0 and id < 200  and name = 'name3' then
	into tab1_part_part23(a,name,id) values(a,name,id)
   when id >0 and id < 200  and name = 'name2' then
	into tab1_part_part22(a,name,id) values(a,name,id)
   when id >0 and id < 200  and name = 'name1' then
     into tab1_part_part21(a,name,id) values(a,name,id)
   when id >0 and id < 200 then
	into tab1_part_part2(a,name,id) values(a,name,id)

   --
   when id < 0 and a > 2000 then
	into tab1_part_part13(a,name,id) values(a,name,id)
   when id < 0 and a > 0 then
	into tab1_part_part12(a,name,id) values(a,name,id)
   when id < 0 and a < 0 then
     into tab1_part_part11(a,name,id) values(a,name,id)
   when id < 0 then
     into tab1_part_part1(a,name,id) values(a,name,id)
   when true then
      into tab1_part(a,name,id) values(a,name,id)

   when a > 1000 and id  > 2000 then
	into vtest1_part_part43(a,name,id) values(a,name,id)
   when a > 1000 and id > 0 then
	into vtest1_part_part42(a,name,id) values(a,name,id)
   when a > 1000 and id < 0 then
	into vtest1_part_part41(a,name,id) values(a,name,id)
   when a > 1000 then
	into vtest1_part_part4(a,name,id) values(a,name,id)
   --
   when a > 200 and a < 1000 and id  > 2000 then
	  into vtest1_part_part33(a,name,id) values(a,name,id)
   when a > 200 and a < 1000 and id  > 0 then
     into vtest1_part_part32(a,name,id) values(a,name,id)
   when a > 200 and a < 1000 and id  < 0 then
	into vtest1_part_part31(a,name,id) values(a,name,id)
   when a > 200 and a < 1000  then
	into vtest1_part_part3(a,name,id) values(a,name,id)

   ---
   when a > 0 and a < 200  and name = 'name3' then
	into vtest1_part_part23(a,name,id) values(a,name,id)
   when a > 0 and a < 200  and name = 'name2' then
	into vtest1_part_part22(a,name,id) values(a,name,id)
   when a > 0 and a < 200  and name = 'name1' then
     into vtest1_part_part21(a,name,id) values(a,name,id)
   when a > 0 and a < 200 then
     into vtest1_part_part2(a,name,id) values(a,name,id)
   --
   when a < 0 and id > 2000 then
	into vtest1_part_part13(a,name,id) values(a,name,id)
   when a < 0 and id > 0 then
	into vtest1_part_part12(a,name,id) values(a,name,id)
   when a < 0 and id < 0 then
	into vtest1_part_part11(a,name,id) values(a,name,id)
   when a < 0 then
	into vtest1_part_part1(a,name,id) values(a,name,id)

   when true then
     into vtest1_part(a,name,id) values(a,name,id)
select * from test_source order by id,name,a;


--ok
insert all
   when id > 1000 and a > 2000 then
		into tab1_part_part43(a,name,id) values(a,name,id)
   when id > 1000 and a > 0 and a <= 2000 then
		into tab1_part_part42(a,name,id) values(a,name,id)
   when id > 1000 and a < 0 then
		into tab1_part_part41(a,name,id) values(a,name,id)
   when id > 1000 then
		into tab1_part_part4(a,name,id) values(a,name,id)
   --
   when id > 200 and id < 1000 and a > 2000 then
	into tab1_part_part33(a,name,id) values(a,name,id)
   when id > 200 and id < 1000 and a > 0 and a <= 2000 then
	into tab1_part_part32(a,name,id) values(a,name,id)
   when id > 200 and id < 1000 and a < 0 then
	into tab1_part_part31(a,name,id) values(a,name,id)
   when id > 200 and id < 1000 then
	into tab1_part_part3(a,name,id) values(a,name,id)

   ---
   when id >0 and id < 200  and name = 'name3' then
	into tab1_part_part23(a,name,id) values(a,name,id)
   when id >0 and id < 200  and name = 'name2' then
	into tab1_part_part22(a,name,id) values(a,name,id)
   when id >0 and id < 200  and name = 'name1' then
     into tab1_part_part21(a,name,id) values(a,name,id)
   when id >0 and id < 200 then
	into tab1_part_part2(a,name,id) values(a,name,id)

   --
   when id < 0 and a > 2000 then
	into tab1_part_part13(a,name,id) values(a,name,id)
   when id < 0 and a > 0 and a <= 2000 then
	into tab1_part_part12(a,name,id) values(a,name,id)
   when id < 0 and a < 0 then
     into tab1_part_part11(a,name,id) values(a,name,id)
   when id < 0 then
     into tab1_part_part1(a,name,id) values(a,name,id)
   when true then
      into tab1_part(a,name,id) values(a,name,id)

   when a > 1000 and id  > 2000 then
	into vtest1_part_part43(a,name,id) values(a,name,id)
   when a > 1000 and id > 0 and id <= 2000 then
	into vtest1_part_part42(a,name,id) values(a,name,id)
   when a > 1000 and id < 0 then
	into vtest1_part_part41(a,name,id) values(a,name,id)
   when a > 1000 then
	into vtest1_part_part4(a,name,id) values(a,name,id)
   --
   when a > 200 and a < 1000 and id  > 2000 then
	  into vtest1_part_part33(a,name,id) values(a,name,id)
   when a > 200 and a < 1000 and id  > 0 and id <= 2000 then
     into vtest1_part_part32(a,name,id) values(a,name,id)
   when a > 200 and a < 1000 and id  < 0 then
	into vtest1_part_part31(a,name,id) values(a,name,id)
   when a > 200 and a < 1000  then
	into vtest1_part_part3(a,name,id) values(a,name,id)

   ---
   when a > 0 and a < 200  and name = 'name3' then
	into vtest1_part_part23(a,name,id) values(a,name,id)
   when a > 0 and a < 200  and name = 'name2' then
	into vtest1_part_part22(a,name,id) values(a,name,id)
   when a > 0 and a < 200  and name = 'name1' then
     into vtest1_part_part21(a,name,id) values(a,name,id)
   when a > 0 and a < 200 then
     into vtest1_part_part2(a,name,id) values(a,name,id)
   --
   when a < 0 and id > 2000 then
	into vtest1_part_part13(a,name,id) values(a,name,id)
   when a < 0 and id > 0 and id <= 2000 then
	into vtest1_part_part12(a,name,id) values(a,name,id)
   when a < 0 and id < 0 then
	into vtest1_part_part11(a,name,id) values(a,name,id)
   when a < 0 then
	into vtest1_part_part1(a,name,id) values(a,name,id)

   when true then
     into vtest1_part(a,name,id) values(a,name,id)
select * from test_source order by id,name,a;

select * from vtest1_part order by a,name,id;

select * from vtest1_part_part1 order by a,name,id;
select * from vtest1_part_part11 order by a,name,id;
select * from vtest1_part_part12 order by a,name,id;
select * from vtest1_part_part13 order by a,name,id;

select  *from vtest1_part_part2 order by a,name,id;
select  *from vtest1_part_part21 order by a,name,id;
select  *from vtest1_part_part22 order by a,name,id;
select  *from vtest1_part_part23 order by a,name,id;

select * from vtest1_part_part3 order by a,name,id;
select  *from vtest1_part_part31 order by a,name,id;
select  *from vtest1_part_part32 order by a,name,id;
select  *from vtest1_part_part33 order by a,name,id;


select * from tab1_part order by a,name,id;

select * from tab1_part_part1 order by a,name,id;
select * from tab1_part_part11 order by a,name,id;
select * from tab1_part_part12 order by a,name,id;
select * from tab1_part_part13 order by a,name,id;

select  *from tab1_part_part2 order by a,name,id;
select  *from tab1_part_part21 order by a,name,id;
select  *from tab1_part_part22 order by a,name,id;
select  *from tab1_part_part23 order by a,name,id;

select * from tab1_part_part3 order by a,name,id;
select  *from tab1_part_part31 order by a,name,id;
select  *from tab1_part_part32 order by a,name,id;
select  *from tab1_part_part33 order by a,name,id;


delete from vtest1_part;
delete from tab1_part;

insert first
   when id > 1000 and a > 2000 then
		into tab1_part_part43(a,name,id) values(a,name,id)
   when id > 1000 and a > 0 then
		into tab1_part_part42(a,name,id) values(a,name,id)
   when id > 1000 and a < 0 then
		into tab1_part_part41(a,name,id) values(a,name,id)
   when id > 1000 then
		into tab1_part_part4(a,name,id) values(a,name,id)
   --
   when id > 200 and id < 1000 and a > 2000 then
	into tab1_part_part33(a,name,id) values(a,name,id)
   when id > 200 and id < 1000 and a > 0 then
	into tab1_part_part32(a,name,id) values(a,name,id)
   when id > 200 and id < 1000 and a < 0 then
	into tab1_part_part31(a,name,id) values(a,name,id)
   when id > 200 and id < 1000 then
	into tab1_part_part3(a,name,id) values(a,name,id)

   ---
   when id >0 and id < 200  and name = 'name3' then
	into tab1_part_part23(a,name,id) values(a,name,id)
   when id >0 and id < 200  and name = 'name2' then
	into tab1_part_part22(a,name,id) values(a,name,id)
   when id >0 and id < 200  and name = 'name1' then
     into tab1_part_part21(a,name,id) values(a,name,id)
   when id >0 and id < 200 then
	into tab1_part_part2(a,name,id) values(a,name,id)

   --
   when id < 0 and a > 2000 then
	into tab1_part_part13(a,name,id) values(a,name,id)
   when id < 0 and a > 0 then
	into tab1_part_part12(a,name,id) values(a,name,id)
   when id < 0 and a < 0 then
     into tab1_part_part11(a,name,id) values(a,name,id)
   when id < 0 then
     into tab1_part_part1(a,name,id) values(a,name,id)
   when true then
      into tab1_part(a,name,id) values(a,name,id)

   when a > 1000 and id  > 2000 then
	into vtest1_part_part43(a,name,id) values(a,name,id)
   when a > 1000 and id > 0 then
	into vtest1_part_part42(a,name,id) values(a,name,id)
   when a > 1000 and id < 0 then
	into vtest1_part_part41(a,name,id) values(a,name,id)
   when a > 1000 then
	into vtest1_part_part4(a,name,id) values(a,name,id)
   --
   when a > 200 and a < 1000 and id  > 2000 then
	  into vtest1_part_part33(a,name,id) values(a,name,id)
   when a > 200 and a < 1000 and id  > 0 then
     into vtest1_part_part32(a,name,id) values(a,name,id)
   when a > 200 and a < 1000 and id  < 0 then
	into vtest1_part_part31(a,name,id) values(a,name,id)
   when a > 200 and a < 1000  then
	into vtest1_part_part3(a,name,id) values(a,name,id)

   ---
   when a > 0 and a < 200  and name = 'name3' then
	into vtest1_part_part23(a,name,id) values(a,name,id)
   when a > 0 and a < 200  and name = 'name2' then
	into vtest1_part_part22(a,name,id) values(a,name,id)
   when a > 0 and a < 200  and name = 'name1' then
     into vtest1_part_part21(a,name,id) values(a,name,id)
   when a > 0 and a < 200 then
     into vtest1_part_part2(a,name,id) values(a,name,id)
   --
   when a < 0 and id > 2000 then
	into vtest1_part_part13(a,name,id) values(a,name,id)
   when a < 0 and id > 0 then
	into vtest1_part_part12(a,name,id) values(a,name,id)
   when a < 0 and id < 0 then
	into vtest1_part_part11(a,name,id) values(a,name,id)
   when a < 0 then
	into vtest1_part_part1(a,name,id) values(a,name,id)

   when true then
     into vtest1_part(a,name,id) values(a,name,id)
select * from test_source order by id,name,a;

select * from vtest1_part order by a,name,id;

select * from vtest1_part_part1 order by a,name,id;
select * from vtest1_part_part11 order by a,name,id;
select * from vtest1_part_part12 order by a,name,id;
select * from vtest1_part_part13 order by a,name,id;

select  *from vtest1_part_part2 order by a,name,id;
select  *from vtest1_part_part21 order by a,name,id;
select  *from vtest1_part_part22 order by a,name,id;
select  *from vtest1_part_part23 order by a,name,id;

select * from vtest1_part_part3 order by a,name,id;
select  *from vtest1_part_part31 order by a,name,id;
select  *from vtest1_part_part32 order by a,name,id;
select  *from vtest1_part_part33 order by a,name,id;


select * from tab1_part order by a,name,id;

select * from tab1_part_part1 order by a,name,id;
select * from tab1_part_part11 order by a,name,id;
select * from tab1_part_part12 order by a,name,id;
select * from tab1_part_part13 order by a,name,id;

select  *from tab1_part_part2 order by a,name,id;
select  *from tab1_part_part21 order by a,name,id;
select  *from tab1_part_part22 order by a,name,id;
select  *from tab1_part_part23 order by a,name,id;

select * from tab1_part_part3 order by a,name,id;
select  *from tab1_part_part31 order by a,name,id;
select  *from tab1_part_part32 order by a,name,id;
select  *from tab1_part_part33 order by a,name,id;

CREATE TABLE tab1_part_alter (name varchar2(256), id integer) partition by range(id);

create table tab1_part_alter_tab1(name varchar2(256),id integer);
create table tab1_part_alter_tab2(name varchar2(256), id integer);

alter table tab1_part_alter attach partition  tab1_part_alter_tab1 for values from (minvalue) to (200);
alter table tab1_part_alter attach partition tab1_part_alter_tab2 for values from (200) to (maxvalue);

insert into test_source values(200,'name1',1);

insert all
  into tab1_part_alter
  into tab1_part_alter
select name,id from test_source;

insert first
  when id < 200 then
	into tab1_part_alter_tab1
  when id > 200 then
	into tab1_part_alter_tab2
  else
    into tab1_part_alter
select name,id from test_source;

select * from tab1_part_alter order by id,name;
select * from tab1_part_alter_tab1 order by id,name;
select  *from tab1_part_alter_tab2 order by id,name;

truncate table tab1_part_alter;
truncate table tab1_part_alter_tab1;
truncate table tab1_part_alter_tab2;

--default is all
insert
  when id > 200 then
	into tab1_part_alter
  when id > 400 then
	into tab1_part_alter
  else
    into tab1_part_alter
select name,id from test_source;


select * from tab1_part_alter order by id,name;
select * from tab1_part_alter_tab1 order by id,name;
select  *from tab1_part_alter_tab2 order by id,name;

truncate table tab1_part_alter;
truncate table tab1_part_alter_tab1;
truncate table tab1_part_alter_tab2;

insert all
  when id > 200 then
	into tab1_part_alter
  when id > 400 then
	into tab1_part_alter
  else
    into tab1_part_alter
select name,id from test_source;


select * from tab1_part_alter order by id,name;
select * from tab1_part_alter_tab1 order by id,name;
select  *from tab1_part_alter_tab2 order by id,name;


truncate table tab1_part_alter;
truncate table tab1_part_alter_tab1;
truncate table tab1_part_alter_tab2;


insert all
  when id > 200 then
	into tab1_part_alter
	into tab1_part_alter
	into tab1_part_alter
  when id > 400 then
	into tab1_part_alter
	into tab1_part_alter
	into tab1_part_alter
  else
    into tab1_part_alter
	into tab1_part_alter
	into tab1_part_alter
select name,id from test_source;

explain (costs off)
insert first
  when id < 200 then
	into tab1_part_alter_tab1
	into tab1_part_alter_tab1
	into tab1_part_alter_tab1
	into tab1_part_alter_tab1
  when id > 200 then
	into tab1_part_alter_tab2
	into tab1_part_alter_tab2
	into tab1_part_alter_tab2
	into tab1_part_alter_tab2
  else
    into tab1_part_alter
	into tab1_part_alter
	into tab1_part_alter
	into tab1_part_alter
select name,id from test_source;


explain (costs off)
insert ALL
  when id < 200 then
	into tab1_part_alter_tab1
	into tab1_part_alter_tab1
	into tab1_part_alter_tab1
	into tab1_part_alter_tab1
  when id > 200 then
	into tab1_part_alter_tab2
	into tab1_part_alter_tab2
	into tab1_part_alter_tab2
	into tab1_part_alter_tab2
  else
    into tab1_part_alter
	into tab1_part_alter
	into tab1_part_alter
	into tab1_part_alter
select name,id from test_source;



--clean data
drop table vtest1_part;
drop table tab1_part;
drop table test_source;
drop table tab1_part_alter;

--test rewritey use default VALUE
create table test_target1(id integer,name varchar2(256) default 'xiexie',salary number default 35555.0);
create table test_target2(id integer default 23,name varchar2(256),salary number default 35565.0);

create table test_source(id integer,name varchar2(256),salary_number number);

insert into test_source values(1,'liming',25555);
insert into test_source  values(2,'xiangbing',35555);
insert into test_source values(3,'lihao',45555);
insert into test_source values(4,'jinyu',56456);

insert ALL
  into test_target1
  into test_target1(id) values(id)
  into test_target1(id,salary) values(id,salary_number)
  into test_target1(id,name,salary)
  into test_target2(name) values(name)
  into test_target2(id,name,salary) values(id,name,salary_number)
  into test_target2
  into test_target2(salary) values(salary_number + 1)
select * from test_source order by id;

select * from test_target1 order by id,name,salary;
select * from test_target2 order by id,name,salary;


truncate table test_target1;
truncate table test_target2;

insert ALL
 when id > 1 then
  into test_target1
  into test_target1(id) values(id)
when id > 2 then
  into test_target1(id,salary) values(id,salary_number)
  into test_target1(id,name,salary)
  into test_target2(name) values(name)
  into test_target2(id,salary,name) values(id,salary_number,name)
when id > 3 then
  into test_target2
  into test_target2(salary) values(salary_number + 1)
select * from test_source order by id;


select * from test_target1 order by id,name,salary;
select * from test_target2 order by id,name,salary;

truncate table test_target1;
truncate table test_target2;

insert first
 when id > 1 then
  into test_target1
  into test_target1(id) values(id)
when id > 2 then
  into test_target1(id,salary) values(id,salary_number)
  into test_target1(id,name,salary)
  into test_target2(name) values(name)
  into test_target2(id,salary,name) values(id,salary_number,name)
when id > 3 then
  into test_target2
  into test_target2(salary) values(salary_number + 1)
select * from test_source order by id;


select * from test_target1 order by id,name,salary;
select * from test_target2 order by id,name,salary;

truncate table test_target1;
truncate table test_target2;

create view test_source_view as select name,salary_number,id from test_source;

insert ALL
 when id > 1 then
  into test_target1
  into test_target1(id) values(id)
when id > 2 then
  into test_target1(id,salary) values(id,salary_number)
  into test_target1(id,name,salary)
  into test_target2(name) values(name)
  into test_target2(id,salary,name) values(id,salary_number,name)
when id > 3 then
  into test_target2
  into test_target2(salary) values(salary_number + 1)
select b.id,b.name,b.salary_number from test_source a left join test_source_view b ON
  a.id = b.id;

select * from test_target1 order by id,name,salary;
select * from test_target2 order by id,name,salary;


truncate table test_target1;
truncate table test_target2;


insert ALL
 when id > 1 then
  into test_target1
  into test_target1(id) values(id)
when id > 2 then
  into test_target1(id,salary) values(id,salary_number)
  into test_target1(id,name,salary)
  into test_target2(name) values(name)
  into test_target2(id,salary,name) values(id,salary_number,name)
when id > 3 then
  into test_target2
  into test_target2(salary) values(salary_number + 1)
select * from (select b.id,b.name,b.salary_number from test_source a left join test_source_view b ON
  a.id = b.id) a;

select * from test_target1 order by id,name,salary;
select * from test_target2 order by id,name,salary;

--clean data
drop table test_target1;
drop table test_target2;
drop view test_source_view;
drop table test_source;

--test transrels
CREATE TABLE tab1_part_alter (name varchar2(256), id integer) partition by range(id);
create table test_source(id integer,name varchar2(256));

create table tab1_part_alter_tab1(name varchar2(256),id integer);
create table tab1_part_alter_tab2(name varchar2(256), id integer);

alter table tab1_part_alter attach partition  tab1_part_alter_tab1 for values from (minvalue) to (200);
alter table tab1_part_alter attach partition tab1_part_alter_tab2 for values from (200) to (maxvalue);

insert into test_source values(200,'name1');
insert into test_source values(-1,'name2');
insert into test_source values(300,'name3');
insert into test_source values(400, 'name4');

create table tab1_part_common(name varchar2(256),id integer);

create or replace function fn_limit_insert_rows() returns trigger LANGUAGE plisql as $$
DECLARE
  new_count integer;
BEGIN
  select count(*) into new_count from new_tbl;
  raise info 'tg_relname = %,tg_level = %,new_count = %', tg_relname, tg_level,new_count;
  raise info 'new = %', new;
  return NULL;
end;
$$;
/


create trigger trg_limit_product_after_insert_row after insert on tab1_part_common referencing
new table as new_tbl for each row execute function fn_limit_insert_rows();
/

create trigger trg_limit_product_after_insert1 after insert on tab1_part_common referencing
new table as new_tbl for each statement execute function fn_limit_insert_rows();
/

create trigger trg_limit_product_after_insert after insert on tab1_part_alter referencing
new table as new_tbl for each STATEMENt execute function fn_limit_insert_rows();
/

insert ALl
    into tab1_part_alter(id,name)
	into tab1_part_alter(name,id) values(name,id)
	into tab1_part_common(id,name)
	into tab1_part_common(name,id) values(name,id)
select * from test_source order by id;


--clean data
drop table tab1_part_alter;
drop table tab1_part_common;
drop function fn_limit_insert_rows;


--test interval partition
CREATE TABLE i_partitioned (
        a number,
		name varchar2(256)
) PARTITION BY RANGE (a) INTERVAL (10);

CREATE TABLE i_partitioned_init PARTITION OF i_partitioned FOR VALUES FROM (-100) TO (0);

create or replace function fn_limit_insert_rows() returns trigger LANGUAGE plisql as $$
BEGIN
  raise info 'tg_relname = %,tg_level = %', tg_relname, tg_level;
  raise info 'new = %', new;
  return NULL;
end;
$$;
/

create trigger trg_limit_product_after_insert_row after insert on i_partitioned
 for each row execute function fn_limit_insert_rows();
/

create trigger trg_limit_product_after_insert1 after insert on i_partitioned referencing
new table as new_tbl for each statement execute function fn_limit_insert_rows();
/


insert into test_source values(250,'name1');
insert into test_source values(-1,'name2');
insert into test_source values(250,'name3');
insert into test_source values(630, 'name4');



insert all
    when id < 1 then
		into i_partitioned(a,name)
    when id > 200 then
		into i_partitioned(name,a) values(name,id)
		into i_partitioned(a,name)
	when true then
		into i_partitioned(name,a) values(name,id)
select * from test_source order by id,name;


--clean data
drop table test_source;
drop table i_partitioned;
drop function fn_limit_insert_rows;


--test values_clause as source
create table test_mds(id integer,name varchar2(256));

--ok
insert all
 into test_mds values(1,'fsjf')
(values (2,'xiexie'),(null,'sjfs'),(1,'dfsf'));

--ok
insert all
 into test_mds values(1,'fsjf')
values (2,'xiexie'),(null,'sjfs'),(1,'dfsf');

--ok
insert all
 into test_mds(name,id) values(column2,column1)
values (2,'xiexie'),(null,'sjfs'),(1,'dfsf');

--ok
insert all
  when column1 > 1 then
		into test_mds(name,id) values(column2,column1)
values (2,'xiexie'),(null,'sjfs'),(1,'dfsf');

--ok
insert all
  when column1 > 1 then
		into test_mds
		into test_mds(id,name)
(values (2,'xiexie'),(null,'sjfs'),(1,'dfsf'));


--ok
insert all
  when column1 > 1 then
		into test_mds
		into test_mds(id,name) values(column1,column2)
values (2,'xiexie');

select * from test_mds order by id,name;

--clean data
drop table test_mds;
