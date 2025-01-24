set ivorysql.enable_ora_default_collation = on;

-- Begin BUG#Z1461
---- 1
CREATE TABLE employees (
    employee_id INT,
    first_name VARCHAR(50),
    last_name VARCHAR(50)
);

INSERT INTO employees (employee_id, first_name, last_name) VALUES (1, 'Alice', 'Smith');
INSERT INTO employees (employee_id, first_name, last_name) VALUES (2, 'Bob', 'Johnson');
INSERT INTO employees (employee_id, first_name, last_name) VALUES (3, 'charlie', 'Brown');
INSERT INTO employees (employee_id, first_name, last_name) VALUES (4, 'CHARLIE', 'Brown');

ALTER SESSION SET NLS_COMP = 'ANSI';
ALTER SESSION SET NLS_SORT = 'BINARY_CI';

-- result is 'CHARLIE' and 'charlie'
SELECT first_name
FROM employees
WHERE first_name = 'CHARLIE'
UNION
SELECT first_name
FROM employees
WHERE first_name = 'charlie';

reset NLS_COMP;
reset NLS_SORT;
DROP TABLE employees;

---- 2
create table tbl_a(val varchar2(10));
create table tbl_b(val varchar2(10));
create table tbl_c as select * from tbl_a union select * from tbl_b;
-- tbl.val's collation must be using_nls_comp
\d tbl_c

create table tbl_d as select val collate binary from tbl_a union select val collate binary from tbl_b;
-- tbl.val's collation must be binary
\d tbl_d
drop table tbl_a, tbl_b, tbl_c, tbl_d;
-- End BUG#Z1461

-- Begin BUG#Z1462
CREATE TABLE employees (
    employee_id INT,
    first_name VARCHAR(50),
    last_name VARCHAR(50)
);

INSERT INTO employees (employee_id, first_name, last_name) VALUES (1, 'Alice', 'Smith');
INSERT INTO employees (employee_id, first_name, last_name) VALUES (2, 'Bob', 'Johnson');
INSERT INTO employees (employee_id, first_name, last_name) VALUES (3, 'charlie', 'Brown');
INSERT INTO employees (employee_id, first_name, last_name) VALUES (4, 'CHARLIE', 'Brown');

ALTER SESSION SET NLS_COMP = 'ANSI';
ALTER SESSION SET NLS_SORT = 'BINARY_CI';

-- result is 'CHARLIE' and 'charlie'
SELECT first_name
FROM employees
WHERE first_name = 'CHARLIE'
INTERSECT
SELECT first_name
FROM employees
WHERE first_name = 'charlie';

reset NLS_COMP;
reset NLS_SORT;
DROP TABLE employees;
-- End BUG#Z1462

-- Begin BUG#Z1463
CREATE TABLE employees (
    employee_id INT,
    first_name VARCHAR(50),
    last_name VARCHAR(50)
);

INSERT INTO employees (employee_id, first_name, last_name) VALUES (1, 'Alice', 'Smith');
INSERT INTO employees (employee_id, first_name, last_name) VALUES (2, 'Bob', 'Johnson');
INSERT INTO employees (employee_id, first_name, last_name) VALUES (3, 'charlie', 'Brown');
INSERT INTO employees (employee_id, first_name, last_name) VALUES (4, 'CHARLIE', 'Brown');

ALTER SESSION SET NLS_COMP = 'LINGUISTIC';
ALTER SESSION SET NLS_SORT = 'BINARY_CI';

-- result is 0,0,3,3
select instr(first_name,'ar') from employees ;

reset NLS_COMP;
reset NLS_SORT;
DROP TABLE employees;
-- End BUG#Z1463

-- Begin BUG#Z1468
create table test(val varchar2(10));
insert into test values ('AbCdEfG');

ALTER SESSION SET NLS_COMP = 'LINGUISTIC';
ALTER SESSION SET NLS_SORT = 'BINARY_CI';

-- result is 'AbCdafG'
select translate(val,'e', 'a') from test;

reset NLS_COMP;
reset NLS_SORT;
drop table test;
-- End BUG#Z1468

-- Begin BUG#Z1473
CREATE TABLE test_table (
    id INT,
    col_name VARCHAR(50)
);

INSERT INTO test_table (id, col_name) VALUES (1, 'B');
INSERT INTO test_table (id, col_name) VALUES (2, 'a');

ALTER SESSION SET NLS_COMP = 'ANSI';
ALTER SESSION SET NLS_SORT = 'BINARY_CI';

-- result is 'a'
select max(col_name) from test_table;

reset NLS_COMP;
reset NLS_SORT;
DROP TABLE test_table;
-- End BUG#Z1473

reset ivorysql.enable_ora_default_collation