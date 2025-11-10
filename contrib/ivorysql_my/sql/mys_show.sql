--show variables
show global variables where VARIABLE_NAME = 'sql_mode';
show session variables where VARIABLE_NAME = 'sql_mode';
show global variables where VARIABLE_NAME like 'sql_mod%';
show session variables where VARIABLE_NAME like 'sql_mod%';
show variables where VARIABLE_NAME like 'sql_mod%';
show variables where VARIABLE_NAME like 'sql_mo%';

show global variables where VARIABLE_NAME = 'sql_quote_show_create';
show session variables where VARIABLE_NAME = 'sql_quote_show_create';
show variables where VARIABLE_NAME like 'sql_quote_show_%';
show variables where VARIABLE_NAME like 'sql_quote%';

--show global variables;

--show create database
create database aa;
show create database aa;
set sql_quote_show_create = false;
show create database aa;
show create database if not exists aa;
show create schema aa;
show create schema if not exists aa;
reset sql_quote_show_create;
drop schema aa;

create database aa CHARACTER SET = UTF8;
show create database aa;
show create database if not exists aa;
show create schema aa;
show create schema if not exists aa;
drop schema aa;

create database aa CHARACTER SET UTF8;
show create database aa;
show create database if not exists aa;
show create schema aa;
show create schema if not exists aa;
drop schema aa;

CREATE database aa COLLATE  = "collatetest";
show create database aa;
show create database if not exists aa;
show create schema aa;
show create schema if not exists aa;
drop schema aa;

CREATE database aa COLLATE "collatetest";
show create database aa;
show create database if not exists aa;
show create schema aa;
show create schema if not exists aa;
drop schema aa;

--show databases
create database aa;
create database bb;
create database cc;
show databases where database = 'aa' or database = 'bb' or database = 'cc';
show databases like 'a%';
show databases like 'b%';
show databases like 'cc%';

show schemas where database = 'aa' or database = 'bb' or database = 'cc';
show schemas like 'a%';
show schemas like 'b%';
show schemas like 'cc%';

drop schema aa;
drop schema bb;
drop schema cc;
show databases where database = 'aa' or database = 'bb' or database = 'cc';
show databases like 'a%';
show databases like 'b%';
show databases like 'cc%';

show schemas where database = 'aa' or database = 'bb' or database = 'cc';
show schemas like 'a%';
show schemas like 'b%';
show schemas like 'c%';

--show columns
create table tb(id int);
create table test(id int, name text, addr text);
show full columns from tb;
show full fields in tb;
show full columns from tb from public;
show full columns from tb in public;

show full columns from tb where Field = 'name';
show full columns from tb like 'na%';
show full fields in tb like 'na%';
show full fields in tb where Field = 'name';

show columns from tb from public where Field = 'name';
show columns from tb in public like 'na%';
drop table tb;
drop table test;

--desc table
create table test(id int, name text, addr text);
desc test;
desc test id;
desc test 'na%';
describe test;
describe test id;
describe test 'na%';

create view vtest as select * from test;
desc vtest;
desc vtest id;
desc vtest 'na%';
drop view vtest;
drop table test;

--show create view
--create table tb(id int, name text);
--create view v_tb as select * from tb;
--create view v_tb1 as select id from tb;
--create view v_tb2 as select name from tb;
--show create view v_tb;
--show create view v_tb1;
--show create view v_tb2;
--drop view v_tb;
--drop view v_tb1;
--drop view v_tb2;
--drop table tb;

--create view vtb as select 1 + 1;
--show create view vtb;
--drop view vtb;

--show index
create table tb(id int, name text);
create index on tb(id);

show index from tb;
show indexes from tb;
show keys from tb;

show index in tb;
show indexes in tb;
show keys in tb;

show index from tb from public;
show index from tb from public where "Table" = 'tb';
show index from tb in public;
show index from tb in public where "Table" = 'tb';

show indexes from tb;
show indexes from tb from public where "Table" = 'tb';
show indexes from tb in public;
show indexes from tb in public where "Table" = 'tb';

show keys from tb;
show keys from tb from public where "Table" = 'tb';
show keys from tb in public;
show keys from tb in public where "Table" = 'tb';

drop table tb;

create table tb(id int, name text);
create index tb_i on tb(id) COMMENT 'test test test';
show index from tb;
drop table tb;

--
--show table status
--because have time, time is always change,so don't test these.
--

--create table tb(id int, name text);
--show table status;
--show table status from public;
--show table status in public;
--show table status from public like 'tb';
--show table status in public like 'tb';
--show table status from public where Name = 'tb';
--show table status in public where Name = 'tb';
--drop table tb;

--show tables;
create table tb(id int, name text);
create temporary table tb1(id int, name text);
create table logs (id serial primary key, message text) with (autovacuum_enabled = false);
create foreign data wrapper dummy;
create server s0 foreign data wrapper dummy;
create foreign table ft1 (
	c1 integer options ("param 1" 'val1') not null,
	c2 text options (param2 'val2', param3 'val3') check (c2 <> ''),
	c3 date,
	check (c3 between '1994-01-01'::date and '1994-01-31'::date)
) server s0 options (delimiter ',', quote '"', "be quoted" 'value');
create table tab_part_create (a int) partition by list (a);

show tables where Tables_in_public = 'tb';
show full tables where Tables_in_public = 'tb1';
show full tables from public where Tables_in_public = 'logs';
show full tables in public where Tables_in_public = 'ft1';
show full tables in public where Tables_in_public = 'tab_part_create';

drop table tb;
drop table tb1;
drop table logs;
drop foreign table ft1;
drop server s0;
drop foreign data wrapper dummy;
drop table tab_part_create;

--show triggers
--CREATE TABLE trig_tbl (id int, val text);
--CREATE FUNCTION trig_func()
--RETURNS trigger LANGUAGE plpgsql AS $$
--BEGIN
--  NEW.val := upper(NEW.val);
--  RETURN NEW;
--END;$$;
--CREATE TRIGGER before_ins BEFORE INSERT ON trig_tbl
--FOR EACH ROW EXECUTE FUNCTION trig_func();
--
--SHOW TRIGGERS from public;
--SHOW TRIGGERS in public like 'trig_tbl';
--SHOW TRIGGERS in public like 'trig_tb%';
--SHOW TRIGGERS in public where Trigger = 'before_ins';
--
--drop  TRIGGER before_ins ON trig_tbl;
--drop FUNCTION trig_func;
--drop TABLE trig_tbl;

--fix bug#Z5991
CREATE TABLE IF NOT EXISTS test_table (
     id SERIAL NOT NULL,
     name VARCHAR(100) NOT NULL,
     email VARCHAR(150),
     department VARCHAR(50),
     salary DECIMAL(10,2),
     hire_date DATE,
     create_time TIMESTAMP DEFAULT CURRENT_TIMESTAMP NOT NULL,
     PRIMARY KEY (id, create_time),
     UNIQUE (email, create_time)
) PARTITION BY RANGE (create_time);

CREATE INDEX IF NOT EXISTS idx_test_table_hire_date ON test_table(hire_date);
CREATE INDEX IF NOT EXISTS idx_test_table_department ON test_table(department);
CREATE INDEX IF NOT EXISTS idx_test_table_create_time ON test_table(create_time);

CREATE TABLE IF NOT EXISTS test_table_p2024
PARTITION OF test_table
FOR VALUES FROM ('2024-01-01') TO ('2025-01-01');

CREATE TABLE IF NOT EXISTS test_table_p2025
PARTITION OF test_table
FOR VALUES FROM ('2025-01-01') TO ('2026-01-01');

CREATE TABLE IF NOT EXISTS test_table_pdefault
PARTITION OF test_table
DEFAULT;

SHOW INDEXES FROM test_table;
SHOW INDEXES FROM test_table_p2024;
SHOW INDEXES FROM test_table_p2025;
SHOW INDEXES FROM test_table_pdefault;

--fix bug#Z5983
CREATE SCHEMA IF NOT EXISTS test_sche;
CREATE TABLE test_sche.test_table(
  id INT PRIMARY KEY,
  name VARCHAR(50) NOT NULL,
  age TINYINT,
  create_time TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

CREATE VIEW test_sche.test_view AS SELECT * FROM test_sche.test_table;
CREATE TEMPORARY TABLE test_temp (id INT);

SHOW TABLES FROM test_sche;

DROP VIEW test_sche.test_view;
DROP TABLE test_sche.test_table;
DROP SCHEMA test_sche;
DROP TABLE test_temp;

--fix bug#Z5979
SET GLOBAL sql_mode = E'\\110\\145\\154\\154\\157'::bytea;
show variables like 'sql_mode';

--fix bug#Z5978
SET GLOBAL time_zone = NULL;
SHOW GLOBAL VARIABLES LIKE 'time_zone';

--fix bug#Z5975
CREATE SCHEMA IF NOT EXISTS test_sche;
CREATE TABLE IF NOT EXISTS test_sche.test_table (
    id INT PRIMARY KEY,
    name VARCHAR(100) NOT NULL,
    email VARCHAR(150) UNIQUE,
    department VARCHAR(50),
    salary DECIMAL(10,2),
    hire_date DATE,
    create_time TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

SELECT *
  FROM INFORMATION_SCHEMA.COLUMNS
  WHERE table_name = 'test_table'
  AND table_schema = 'test_sche';

DROP TABLE test_sche.test_table;
DROP SCHEMA test_sche;

--fix bug#Z5974
CREATE TABLE test_table4 (
    id SERIAL PRIMARY KEY,
    btree_col INT NOT NULL,
    hash_col VARCHAR(50) NOT NULL,
    data DATE,
    text_col TEXT,
    jsonb_col JSONB,
    array_col TEXT[],
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

CREATE INDEX idx_btree ON test_table4 USING BTREE(btree_col);
CREATE INDEX idx_hash ON test_table4 USING HASH(hash_col);
CREATE INDEX idx_gist_text ON test_table4 USING GiST(to_tsvector('english', text_col));
CREATE INDEX idx_gin_jsonb ON test_table4 USING GIN(jsonb_col);
CREATE INDEX idx_brin_date ON test_table4 USING BRIN(data);
CREATE INDEX idx_spgist_text ON test_table4 USING SPGiST(text_col);

SHOW INDEXES FROM test_table4;
DROP TABLE test_table4;

--fix bug#Z5969
set session max_index_keys=NULL;
show variables like 'max_index_keys';

-- fix bug#Z5973
drop table test_table;
CREATE TABLE IF NOT EXISTS test_table (
    id INT PRIMARY KEY,
    name VARCHAR(100) NOT NULL,
    email VARCHAR(150) UNIQUE,
    department VARCHAR(50),
    salary DECIMAL(10,2),
    hire_date DATE,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

CREATE VIEW test_view AS SELECT * FROM test_table;

CREATE TEMPORARY TABLE IF NOT EXISTS temp_table (
    id INT PRIMARY KEY,
    name VARCHAR(100) NOT NULL,
    email VARCHAR(150) UNIQUE,
    department VARCHAR(50),
    salary DECIMAL(10,2),
    hire_date DATE,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

SHOW INDEX FROM test_table;
SHOW INDEX FROM test_view;
SHOW INDEX FROM temp_table;
SHOW INDEX FROM temp_table FROM pg_temp_4;

SHOW INDEXES FROM 123abc;
SHOW INDEX FROM test_table FROM 123abc;

drop view test_view;
drop table test_table;
drop table temp_table;

-- fix bug#Z5973
CREATE TABLE IF NOT EXISTS test_table (
    id INT PRIMARY KEY,
    name VARCHAR(100) NOT NULL,
    email VARCHAR(150) UNIQUE,
    department VARCHAR(50),
    salary DECIMAL(10,2),
    hire_date DATE,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

-- fix bug#Z5958
SHOW COLUMNS FROM 123aaabbb;
SHOW COLUMNS FROM test_table LIKE '123aaabbb';
SHOW COLUMNS FROM test_table LIKE 'name';
drop table test_table;

-- fix bug#Z5956
CREATE TEMPORARY TABLE temp_table (
	id SERIAL PRIMARY KEY,
	name VARCHAR(100) NOT NULL,
	email VARCHAR(150) UNIQUE,
	department VARCHAR(50),
	salary DECIMAL(10,2),
	hire_date DATE,
	create_time TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

SHOW COLUMNS FROM temp_table;
SHOW COLUMNS FROM temp_table FROM pg_temp_4;
drop table temp_table;

-- fix bug#5954
CREATE TABLE IF NOT EXISTS test_table (
    id SERIAL PRIMARY KEY,
    name VARCHAR(100) NOT NULL,
    email VARCHAR(150) UNIQUE,
    department VARCHAR(50),
    salary DECIMAL(10,2),
    hire_date DATE,
    create_time TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);
COMMENT ON COLUMN test_table.name IS 'user name';
CREATE VIEW test_view AS SELECT id, name, create_time FROM test_table;
SHOW FULL COLUMNS FROM test_view;

drop view test_view;
drop table test_table;

-- fix bug#5949
CREATE TABLE IF NOT EXISTS test_table (
	id SERIAL PRIMARY KEY,
	name VARCHAR(100) NOT NULL,
	email VARCHAR(150) UNIQUE,
	department VARCHAR(50),
	salary DECIMAL(10,2),
	hire_date DATE,
	create_time TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);
SHOW COLUMNS FROM test_table;
SHOW FULL COLUMNS FROM test_table;

drop table test_table;


-- fix bug#Z5968
CREATE SCHEMA IF NOT EXISTS test_sche;
CREATE TABLE test_sche.test_table(
id INT PRIMARY KEY,
name VARCHAR(50) NOT NULL,
age TINYINT,
create_time TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);
CREATE VIEW test_sche.test_view AS SELECT * FROM test_sche.test_table;
--SHOW TABLE STATUS IN test_sche;  --because of time always change, manual operation is required
SHOW TABLE STATUS IN 123abc;
SHOW TABLE STATUS IN highgoadsfdsf;

drop view test_sche.test_view;
drop table test_sche.test_table;
drop schema test_sche;

-- fix bug#Z5948
CREATE TABLE IF NOT EXISTS test_table (
    id SERIAL PRIMARY KEY,
    name VARCHAR(100) NOT NULL,
    email VARCHAR(150) UNIQUE,
    department VARCHAR(50),
    salary DECIMAL(10,2),
    hire_date DATE,
    create_time TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

CREATE VIEW test_view AS
  SELECT id, name, create_time FROM test_table;

--SHOW CREATE TABLE test_view;		--zh_CN.utf8 is displayed as zh_CN.UTF-8 on some systems
--   View    |                   Create View                   | character_set_client | collation_connection
-- -----------+-------------------------------------------------+----------------------+----------------------
-- test_view | create view test_view as  SELECT test_table.id,+| UTF8                 | zh_CN.utf8
--           |     test_table.name,                           +|                      |
--           |     test_table.create_time                     +|                      |
--           |    FROM test_table;                             |                      |
--(1 row)

drop view test_view;
drop table test_table;

--fix bug#Z5946
CREATE TABLE IF NOT EXISTS test_table (
    id SERIAL PRIMARY KEY,
    name VARCHAR(100) NOT NULL,
    email VARCHAR(150) UNIQUE,
    department VARCHAR(50),
    salary DECIMAL(10,2),
    hire_date DATE,
    create_time TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

CREATE VIEW test_view AS
  SELECT id, name, create_time FROM test_table;

--SHOW CREATE TABLE test_view;		--zh_CN.utf8 is displayed as zh_CN.UTF-8 on some systems
--   View    |                   Create View                   | character_set_client | collation_connection
-- -----------+-------------------------------------------------+----------------------+----------------------
-- test_view | create view test_view as  SELECT test_table.id,+| UTF8                 | zh_CN.utf8
--           |     test_table.name,                           +|                      |
--           |     test_table.create_time                     +|                      |
--           |    FROM test_table;                             |                      |
--(1 row)

drop view test_view;
drop table test_table;


--fix bug#Z5947
CREATE TABLE IF NOT EXISTS test_table (
	id SERIAL PRIMARY KEY,
	name VARCHAR(100) NOT NULL,
	email VARCHAR(150) UNIQUE,
	department VARCHAR(50),
	salary DECIMAL(10,2),
	hire_date DATE,
	create_time TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

SET sql_quote_show_create = 1;
SHOW CREATE TABLE test_table;

drop table test_table;