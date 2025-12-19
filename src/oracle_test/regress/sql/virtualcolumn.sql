--
-- Virtual column test
--

-- Test regular virtual column test
CREATE TABLE vtest1 (a int PRIMARY KEY, b int GENERATED ALWAYS AS (a * 2));
insert into vtest1 (a) values (1);
insert into vtest1(a,b) values (1,2);
update vtest1 set b = 3 where a = 1;
update vtest1 set a = 2 where b = 2;
delete from vtest1 where b = 4;
SELECT * FROM vtest1;
select a, b from vtest1;
DROP TABLE vtest1;

-- Test virtual column as index
CREATE TABLE test_idx (a int, b int GENERATED ALWAYS AS (a * 2) PRIMARY KEY);
insert into test_idx (a) values (1);
select * from test_idx where b = 2;
DROP TABLE test_idx;

CREATE TABLE test_idx2 (a int, b int GENERATED ALWAYS AS (a * 2));
create index gtest1_idx on test_idx2 (b);
insert into test_idx2 (a) values (1);
select * from test_idx2 where b = 2;
DROP TABLE test_idx2;

-- Test check constrains
CREATE TABLE vtest2 (a int PRIMARY KEY, b int GENERATED ALWAYS AS (a * 2) check (b > 5));
insert into vtest2(a) values (1);--error
insert into vtest2(a) values (3);
SELECT * FROM vtest2;
DROP TABLE vtest2;

-- Test Deafault and NULL
CREATE TABLE vtest3 (a int PRIMARY KEY, b int GENERATED ALWAYS AS (a * 2) DEFAULT 5);--error
CREATE TABLE vtest4 (a int PRIMARY KEY, b int GENERATED ALWAYS AS (a * 2) NOT NULL);--ok
insert into vtest4 (a) values (1);
select * from vtest4;
DROP TABLE vtest4;
CREATE TABLE vtest5 (a int PRIMARY KEY, b int GENERATED ALWAYS AS (a * 2) NULL);
insert into vtest5(a) values (3);
SELECT * FROM vtest5;
DROP TABLE vtest5;

-- Can't refrence other virtual column
CREATE TABLE vtest_err_1 (a int PRIMARY KEY, b int GENERATED ALWAYS AS (b * 2));
CREATE TABLE vtest_err_2 (a int PRIMARY KEY, b int GENERATED ALWAYS AS (a * 2), c int GENERATED ALWAYS AS (b * 3));

-- Test not deterministic function as virtual column expr
create or replace function my_func(a int) return int
is
begin
	return a + 5;
end;
/
CREATE TABLE vtest6 (a int PRIMARY KEY, b int GENERATED ALWAYS AS (my_func(a)));
DROP FUNCTION my_func;

-- Test deterministic function as virtual column expr
create or replace function my_func(a int) return int deterministic
is
begin
	return a + 5;
end;
/
CREATE TABLE vtest6 (a int PRIMARY KEY, b int GENERATED ALWAYS AS (my_func(a)));
insert into vtest6 (a) values (1);
SELECT * FROM vtest6;
DROP TABLE vtest6;
DROP FUNCTION my_func;

-- Test ALTER virtual column
create table t1 (id number,product varchar2(50),price number(10,2),price_with_tax int generated always as (round(price*1.2,2)) virtual);
insert into t1 (id, product, price) values (1, 'computer', 1500);
insert into t1 (id, product, price) values (2, 'bike', 1000);
select * from t1;

alter table t1 modify (price_with_tax int as (round(price*1.3,2)));
select * from t1;
alter table t1 drop column price_with_tax;
alter table t1 add (price_with_tax int as (round(price*1.5,2)));
select * from t1;
drop table t1;

create table t2 (id number,product varchar2(50),b int, price number(10,2),price_with_tax int generated always as (round(price*1.2,2)) virtual);
insert into t2 (id,b, product, price) values (1, 5, 'computer', 1500);
insert into t2 (id,b, product, price) values (2, 10, 'bike', 1000);
select * from t2;
alter table t2 modify (b as (round(price*1.3,2)));
drop table t2;

-- Test same virtual column expr
create table vir_tbl (id int,id1 int,a as (id*2) ,b as (id*2));--error
create or replace function my_func(a int) return int deterministic
is
begin
        return a + 5;
end;
/
create table vir_tbl (id int,a as (my_func(id)) ,b as (my_func(id)));--error
drop function my_func;

--test hash join
CREATE TABLE vtest1 (a int PRIMARY KEY, b int GENERATED ALWAYS AS (a * 2));
insert into vtest1 (a) values (1);
insert into vtest1 (a) values (2);

CREATE TABLE vtest2 (a int PRIMARY KEY, b int GENERATED ALWAYS AS (a * 2));
insert into vtest2(a) values (1);
insert into vtest2(a) values (2);
select g1.a,g1.b,g2.a,g2.b from vtest1 g1,vtest2 g2 where g1.a = g2.a;
drop table vtest1;
drop table vtest2;

-- test number type
create table t1 (id number(10,2), price number(10,2) as (id*2));
insert into t1 (id) values (1),(2);
select * from t1;
Drop table t1;

--test invalid column expression
CREATE TABLE gtest6 (a int PRIMARY KEY, b int GENERATED ALWAYS AS (a));--error

--test not support type
CREATE TABLE gtest6 (a CLOB, b CLOB GENERATED ALWAYS AS (a));--error
CREATE TABLE gtest6 (a BLOB, b BLOB GENERATED ALWAYS AS (a));--error

--test temp table
create global temporary table glb_tbl (id number,price  number(10,2) generated always as (id*2) virtual);--error
create temporary table glb_tbl (id number,price  number(10,2) generated always as (id*2) virtual);--error

--test partiton table
CREATE TABLE orders
    (order_id INTEGER,
     order_date DATE,
     customer_id NUMBER(12) as (order_id*2))
PARTITION BY RANGE (customer_id);

CREATE TABLE orders_y2023 PARTITION OF orders
    FOR VALUES FROM (1) TO (10);

CREATE TABLE orders_y2024 PARTITION OF orders
    FOR VALUES FROM (11) TO (20);

insert into orders (order_id,order_date) values(1,current_timestamp);
drop table orders;

CREATE TABLE orders
    (order_id INTEGER,
     order_date DATE,
     customer_id NUMBER(12) as (order_id*2))
PARTITION BY RANGE (order_id);

CREATE TABLE orders_y2023 PARTITION OF orders
    FOR VALUES FROM (1) TO (10);

CREATE TABLE orders_y2024 PARTITION OF orders
    FOR VALUES FROM (11) TO (20);
insert into orders (order_id,order_date) values(1,current_timestamp);
drop table orders;

CREATE TABLE orders (
    id int,
    country TEXT NOT NULL,
    order_date DATE NOT NULL,
    amount NUMERIC(10, 2) as (id*365)
) PARTITION BY LIST (country);

CREATE TABLE orders_germany PARTITION OF orders
    FOR VALUES IN ('Germany');

CREATE TABLE orders_france PARTITION OF orders
    FOR VALUES IN ('France');

CREATE TABLE orders_uk PARTITION OF orders
    FOR VALUES IN ('United Kingdom');

insert into orders (id,country,order_date) values(1,'Germany',current_timestamp);
insert into orders (id,country,order_date) values(2,'France',current_timestamp);
insert into orders (id,country,order_date) values(3,'United Kingdom',current_timestamp);
drop table orders;

CREATE TABLE orders (
    id int,
    customer_id INTEGER NOT NULL,
    order_date DATE NOT NULL,
    amount NUMERIC(10, 2) as (id*10)
) PARTITION BY RANGE (amount);

CREATE TABLE orders_10 PARTITION OF orders
    FOR VALUES FROM (1) TO (100);

CREATE TABLE orders_10000 PARTITION OF orders
    FOR VALUES FROM (101) TO (10000);

CREATE TABLE orders_1000000 PARTITION OF orders
    FOR VALUES FROM (10001) TO (1000000);
insert into orders (id,customer_id,order_date) values(1,10,current_timestamp);
drop table orders;

CREATE TABLE t1 (a int, b int GENERATED ALWAYS AS (a * 2));
insert into t1 (a) values(generate_series(1,10000));
CREATE STATISTICS stats on a,b from t1;
analyze t1;
SELECT m.* FROM pg_statistic_ext join pg_statistic_ext_data on (oid = stxoid), pg_mcv_list_items(stxdmcv) m WHERE stxname = 'stats';
drop STATISTICS stats;
drop table t1;

--Test inherits table
CREATE TABLE v_parent (a int primary key, b int GENERATED ALWAYS AS (a * 2));
CREATE TABLE v_inh (
    state           char(2)
) INHERITS (v_parent);
\d v_inh
insert into v_inh (a, b, state) values (1, 3, 'd');
drop table v_inh;

CREATE TABLE v_like (like v_parent including generated);
\d v_like
insert into v_like (a, b) values (1,3);
drop table v_like;
drop table v_parent;

--Begin - BUG#Z485
create table vir_alt1 (a int);
alter table vir_alt1 add b generated always as (a*3) virtual;
drop table vir_alt1;
--End - BUG#Z485

--Begin - BUG#Z486
create table vir_alt2 (a int);
alter table vir_alt2 add (c  as (a*4), d as (a*5), e as (a*66));
drop table vir_alt2;
--End - BUG#Z486

--Begin - BUG#Z487
create table vir_alt3 (a int, b as (a*2), c as (a*3), d as (a*4));
alter table vir_alt3 modify (b as (a*6), c as (a*7),  d as (a*9));
drop table vir_alt3;
--End - BUG#Z487

--Begin - BUG#Z488
create table vir_cons4_01 (a int, b as (a*2));
alter table vir_cons4_01 alter column b set default 3;
drop table vir_cons4_01;
--End - BUG#Z488

--Begin - BUG#Z489
create table vir_cons8 (a int, b int, c as (b*2) UNIQUE);
insert into vir_cons8 (a, b) values (1,1);
insert into vir_cons8 (a, b) values (2,1);
drop table vir_cons8;
--End - BUG#Z489

--Begin - BUG#Z491
create table vir_cons7 (a int, b int, c as (b*2) not null);
insert into vir_cons7 (a) values (1);
drop table vir_cons7;
--End - BUG#Z491

--Begin - BUG#Z492
create table vir_cons6 (a int, b int, c as (b*2) null);
insert into vir_cons6 (a) values (1);
select * from vir_cons6;
drop table vir_cons6;
--End - BUG#Z492

--Begin - BUG#Z493
create or replace function my_func(a int) return int deterministic
as
begin
	return a+2;
end;
/
create table vir_cite6 (a int, name varchar(100), v_id as (my_func(a))) partition by range (v_id);--error
drop function my_func;
--End - BUG#Z493

--Begin - BUG#Z6871
CREATE TABLE test_tab (a number, b int GENERATED ALWAYS AS (a * 2), name varchar2(256), id number);
select * from test_tab order by a,name,id;
drop table test_tab;
CREATE TABLE test_tab (a number, b int GENERATED ALWAYS AS (2), name varchar2(256), id number);
select * from test_tab order by a,name,id;
drop table test_tab;
--End - BUG#Z6871