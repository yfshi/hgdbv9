--
-- visibility column test
--
CREATE TABLE tab1 (id NUMBER,description VARCHAR2(50) INVISIBLE);
\d tab1
INSERT INTO tab1 VALUES (1);
SELECT * FROM tab1;
INSERT INTO tab1 (id, description) VALUES (2, 'TWO');
SELECT id, description FROM tab1;
SET COLINVISIBLE to ON;
\d tab1
SET COLINVISIBLE to OFF;
drop table tab1;

CREATE TABLE tab1 (id NUMBER,description VARCHAR2(50) INVISIBLE);
insert into tab1 values(1,'one');--error
drop table tab1;

CREATE TABLE tab1 (a NUMBER,b NUMBER,c NUMBER INVISIBLE);
INSERT INTO tab1 (a, b, c)VALUES (1 , 1 , 1);
ALTER TABLE tab1 MODIFY b INVISIBLE;
ALTER TABLE tab1 MODIFY c VISIBLE;
SELECT * FROM tab1;
ALTER TABLE tab1 MODIFY (b INVISIBLE,c INVISIBLE);
SELECT * FROM tab1;
ALTER TABLE tab1 MODIFY (b VISIBLE,c INVISIBLE);
SELECT * FROM tab1;
drop table tab1;

create table inv_cre1 (id int, name varchar(100));
insert into inv_cre1 (id, name) values (1, '张三');
alter table inv_cre1 add age int invisible;
insert into inv_cre1 (id, name, age) values (2, '李四', 23);
select id, name, age from inv_cre1;
\d inv_cre1
drop table inv_cre1;

CREATE TABLE orders_vsb
    (order_id INTEGER,
     order_date DATE,
     customer_id NUMBER(12) invisible)
PARTITION BY RANGE (customer_id);

CREATE TABLE orders_vsb_10 PARTITION OF orders_vsb
    FOR VALUES FROM (1) TO (10);

CREATE TABLE orders_vsb_20 PARTITION OF orders_vsb
    FOR VALUES FROM (11) TO (20);

insert into orders_vsb (order_id,order_date,customer_id) values(1,'2023-07-17'::date,5);
insert into orders_vsb (order_id,order_date,customer_id) values(1,'2023-07-17'::date,12);
select * from orders_vsb;
select *,customer_id from orders_vsb;
\d orders_vsb
\d orders_vsb_10
\d orders_vsb_20
set colinvisible to on;
\d orders
\d orders_vsb_10
\d orders_vsb_20
set colinvisible to off;
drop table orders_vsb;


CREATE TABLE tab1 (id NUMBER,description VARCHAR2(50) INVISIBLE);
INSERT INTO tab1 (id,description) VALUES (1,'one');
INSERT INTO tab1 (id,description) VALUES (2,'two');
create view tab1_v1 as select * from tab1;
create view tab1_v2 as select id,description from tab1;
select * from tab1_v1;
\d tab1_v1
select id,description from tab1_v1;--error
select * from tab1_v2;
\d tab1_v2
drop view tab1_v1;
drop view tab1_v2;
drop table tab1;


CREATE TABLE orders_vsb
    (order_id INTEGER,
     order_date DATE,
     customer_id NUMBER(12) invisible)
PARTITION BY RANGE (customer_id);

CREATE TABLE orders_vsb_10 PARTITION OF orders_vsb
    FOR VALUES FROM (1) TO (10);

CREATE TABLE orders_vsb_20 PARTITION OF orders_vsb
    FOR VALUES FROM (11) TO (20);

insert into orders_vsb (order_id,order_date,customer_id) values(1,'2023-07-17'::date,5);
insert into orders_vsb (order_id,order_date,customer_id) values(1,'2023-07-17'::date,12);
select * from orders_vsb;
select *,customer_id from orders_vsb;
\d orders_vsb
\d orders_vsb_10
\d orders_vsb_20
set colinvisible to on;
\d orders
\d orders_vsb_10
\d orders_vsb_20
drop table orders_vsb;
set colinvisible to off;

--test rowtype
CREATE TABLE tab1 (id NUMBER,description VARCHAR2(50) INVISIBLE);
insert into tab1(id,description) values(1,'one');
CREATE OR REPLACE FUNCTION get_employee(emp_id NUMBER)
   RETURN VARCHAR2
IS
	r_emp tab1%ROWTYPE;
BEGIN
	select * into r_emp FROM tab1 WHERE id = emp_id;
	raise info '信息:%', r_emp.description;
	RETURN r_emp.description;
END;
/
select get_employee(1) from dual;--error
drop function get_employee;
drop table tab1;

--test user-define type
CREATE TYPE address AS (
    street  TEXT,
    city    TEXT,
    state   CHAR(2),
    zip     INTEGER
);
CREATE TABLE customers (
    id        int,
    name      TEXT,
    address   address INVISIBLE
);--error
drop type address;

create table inv_vir1 (a int, b invisible as (a*2));
insert into inv_vir1 (a) values (1);
select * from inv_vir1;
select a, b from inv_vir1;
drop table inv_vir1;

--join test
drop table if exists tt2;
drop table if exists tt3;
drop table if exists tt4;
create table tt2 (a int, b int invisible, c int);
create table tt3 (ax int, b int invisible, c numeric);
create table tt4 (ay int, b int invisible, q int);
select * from tt2 join tt3 using (b,c) full join tt4 using (b);
drop table tt2;
drop table tt3;
drop table tt4;

--inherits  test
CREATE TABLE vtest1 (a int primary key, b int invisible);
insert into vtest1(a,b) values (1,2);
CREATE TABLE v_inh (
    state           char(2)
) INHERITS (vtest1);
CREATE TABLE vtest1_like (like vtest1);
create table vtest1_as as select a,b from vtest1;
create table vtest1_as_1 as select * from vtest1;
\d v_inh
\d vtest1_like
\d vtest1_as
\d vtest1_as_1
set colinvisible to on;
\d v_inh
\d vtest1_like
\d vtest1_as
\d vtest1_as_1
set colinvisible to off;
drop table vtest1_as_1;
drop table vtest1_as;
drop table vtest1_like;
drop table v_inh;
drop table vtest1;

--Begin - Bug#Z504
CREATE TABLE tab1 (id NUMBER INVISIBLE,description VARCHAR2(50) INVISIBLE);--error
create table inv_alt1 (id int, name varchar(100), age int);
insert into inv_alt1 (id, name, age) values (1, '张三', 23);
alter table inv_alt1 modify (id invisible, name invisible, age invisible);--error
drop table inv_alt1;
--End - Bug#Z504