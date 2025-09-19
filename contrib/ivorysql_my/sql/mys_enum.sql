
set SQL_MODE=default;
create table test(color enum('aaa', 'bbb', 'ccc', 'ddd', 'eee'));
insert into test values(NULL);
insert into test values('');
insert into test values('aaa');
select * from test;
select color+0 from test;
drop table test;

create table t(a enum('0', '1'));
insert into t values ('0');
select a, a+0 from t;
drop table t;

set SQL_MODE='';
create table test(color enum('aaa', 'bbb', 'ccc', 'ddd', 'eee'));
insert into test values(NULL);
insert into test values('');
insert into test values('aaa');
select * from test;
select color+0 from test;
drop table test;

create table t(a enum('0', '1'));
insert into t values ('0');
select a, a+0 from t;
drop table t;

set SQL_MODE=default;
create table test(color enum('aaa', 'bbb', 'ccc', 'ddd', 'eee'));
insert into test values('1.4');
insert into test values('1.5');
insert into test values('1');
insert into test values('0');
insert into test values(1);
insert into test values(1.5);
insert into test values(5);
insert into test values(5.4);
insert into test values(5.5);
insert into test values(10);
insert into test values(0);
select * from test;
drop table test;

set SQL_MODE='';
create table test(color enum('aaa', 'bbb', 'ccc', 'ddd', 'eee'));
insert into test values('1.4');
insert into test values('1.5');
insert into test values('1');
insert into test values('0');
insert into test values(1);
insert into test values(1.5);
insert into test values(5);
insert into test values(5.4);
insert into test values(5.5);
insert into test values(10);
insert into test values(0);
select * from test;
drop table test;

set SQL_MODE=default;
drop table if exists test;
create table test(color enum('red', 'green', 'blue', 'purple', 'yellow'));
insert into test values('red');
insert into test values('green');
insert into test values('orange');
insert into test values(0);
insert into test values('0');
insert into test values(1);
insert into test values('1');
insert into test values(1.2);
insert into test values('1.2');
insert into test values(1.4);
insert into test values('1.4');
insert into test values(1.5);
insert into test values('1.5');
insert into test values(1.6);
insert into test values('1.6');
insert into test values('a1');
insert into test values('1a');
insert into test values('a123');
insert into test values('123a');
select * from test;
delete from test;

set SQL_MODE='';
insert into test values('red');
insert into test values('green');
insert into test values('orange');
insert into test values(0);
insert into test values('0');
insert into test values(1);
insert into test values('1');
insert into test values(1.2);
insert into test values('1.2');
insert into test values(1.4);
insert into test values('1.4');
insert into test values(1.5);
insert into test values('1.5');
insert into test values(1.6);
insert into test values('1.6');
insert into test values('a1');
insert into test values('1a');
insert into test values('a123');
insert into test values('123a');
select * from test;
drop table test cascade;

set SQL_MODE=default;
create table test(id enum('a','b'));
\d test
insert into test values('a');
insert into test values('b');
insert into test values('');
insert into test values(' ');
insert into test values(NULL);
insert into test values('cc');
select * from test;
select id+0 from test;
drop table test;


set SQL_MODE='';
create table test(id enum('a','b'));
\d test
insert into test values('a');
insert into test values('b');
insert into test values('');
insert into test values(' ');
insert into test values(NULL);
insert into test values('cc');
select * from test;
select id+0 from test;
drop table test;


set SQL_MODE to default;
-- oper
--------------------------------------------------
CREATE TABLE t2 (a enum('a','b', 'e'), b enum('a', 'b', 'c','d'));
insert into t2 values('a', 'a');
insert into t2 values('a', 'b');
insert into t2 values('a', 'c');
insert into t2 values('a', 'd');

insert into t2 values('b', 'a');
insert into t2 values('b', 'b');
insert into t2 values('b', 'c');
insert into t2 values('b', 'd');

insert into t2 values('e', 'a');
insert into t2 values('e', 'b');
insert into t2 values('e', 'c');
insert into t2 values('e', 'd');

---=
select * from t2 where a = b;
select * from t2 where b = 999;
--<>
select * from t2 where a <> b;
select * from t2 where b <> 999;
--<
select * from t2 where a < b;
select * from t2 where b < 999;
-->
select * from t2 where a > b;
select * from t2 where b > 999;
--<=
select * from t2 where a <= b;
select * from t2 where b <= 999;
-->=
select * from t2 where a >= b;
select * from t2 where b >= 999;
--<=>
select * from t2 where a <=> b;
select * from t2 where b <=> 999;

--+
select a + b from t2;
select a + 11 from t2;
---
select -a, -b from t2;
select b - a from t2;
select b - 11 from t2;
-- *
select b * a from t2;
select b * 11 from t2;

--"&"
select b & a from t2;
select b & 11 from t2;

--"|"
select b | a from t2;
select b | 11 from t2;

--"~"
select ~b from t2; --18446744073709550616

--"^"
select b ^ a from t2;
select b ^ 11 from t2;

--">>"
select b >> a from t2;
select b >> 11 from t2;
select b >> 2 from t2;

--"<<"
select b << a from t2;
select b << 11 from t2;
select b << 2 from t2;

--"&&"
select b && a from t2;
select b && 11 from t2;

--"||"
select b || a from t2;
select b || 11 from t2;

--"!"
select !b from t2;

set sql_mode='';
insert into t2 values('a', NULL);
insert into t2 values('b', '');
insert into t2 values('c', 'b');
--"and"
select b and a from t2;
select b and 11::boolean from t2;

--"or"
select b or a from t2;
select b or 11::boolean from t2;

--"xor"
select b xor a from t2;
select b xor 11::boolean from t2;
drop table t2;

set sql_mode to default;
CREATE TABLE t2 (a enum('a','b', 'e'), b enum('a', 'b', 'c','d'));
insert into t2 values('a', 'a');
insert into t2 values('a', 'b');
insert into t2 values('b', 'a');
insert into t2 values('b', 'b');
insert into t2 values('e', 'a');
insert into t2 values('e', 'b');

-- /
select b / a from t2;
select b / 11 from t2;
select a / b from t2;
select 0 / a from t2;

--div
select b div a from t2;
select b div 11 from t2;
select a div b from t2;
select 0 div a from t2;

-- %
select b % a from t2;
select b % 11 from t2;
select a % b from t2;
select 0 % a from t2;

--mod
select b mod a from t2;
select b mod 11 from t2;
select a mod b from t2;
select 0 mod a from t2;

set sql_mode='';
-- /
select b / a from t2;
select b / 11 from t2;
select a / b from t2;
select 0 / a from t2;

--div
select b div a from t2;
select b div 11 from t2;
select a div b from t2;
select 0 div a from t2;

-- %
select b % a from t2;
select b % 11 from t2;
select a % b from t2;
select 0 % a from t2;

--mod
select b mod a from t2;
select b mod 11 from t2;
select a mod b from t2;
select 0 mod a from t2;

drop table t2;


set sql_mode = default;
-- between and
CREATE TABLE t2 (id1 enum('aa','bb', 'cc', 'dd'), id2 enum('cc','dd','ee', 'ff'));
insert into t2 values('aa', 'cc');
insert into t2 values('bb', 'cc');
insert into t2 values('dd', 'cc');
select * from t2 where id1 between 'aa' and 'cc';
select * from t2 where id1 between 'aa' and 'ee';
select * from t2 where id1 between 'aa' and 'dddd';
select * from t2 where id1 between 'aa' and NULL;
select * from t2 where id1 between 'aa' and 'ab';
select * from t2 where id1 between 'aa' and 'aab';
select * from t2 where id1 between 'aa' and 'ba';
select * from t2 where id1 between 'aa' and 'bbb';
select * from t2 where id1 between 'aa' and '1';
select * from t2 where id1 between 'aa' and 1;
select * from t2 where id2 >= id1 and id2 <= 555;
select * from t2 where id2 between id1 and 555; -- between and: compare characters   >= : compare idx
--select * from t2 where id2 between id1 and 555;
-- id1 | id2 
-------+-----
-- aa  | cc
--(1 row)
drop table t2;

set sql_mode = default;
-- in
CREATE TABLE t2 (id1 enum('a','b'), id2 enum('c','d'));
insert into t2 values('a', 'c');
select * from t2 where id1 in ('a','h','y');
select * from t2 where id1 not in ('a','h','y');
select * from t2 where id1 in ('z','h',1);
drop table t2;

set sql_mode = '';
CREATE TABLE t2 (id1 enum('a','b'), id2 enum('c','d'));
insert into t2 values('a', 'c');
select * from t2 where id1 in ('a','h','y');
select * from t2 where id1 not in ('a','h','y');
select * from t2 where id1 in ('z','h',1);
drop table t2;

-- is
set SQL_MODE='';
CREATE TABLE t2 (id1 enum('a','b'), id2 enum('c','d'));
insert into t2 values('a', 'c');
insert into t2 values('a', ' ');
insert into t2 values('a', NULL);
SELECT * FROM t2 WHERE id1 IS true;
SELECT * FROM t2 WHERE id2 IS true;
SELECT * FROM t2 WHERE id2 IS not true;
SELECT * FROM t2 WHERE id2 IS NULL;
SELECT * FROM t2 WHERE id2 IS not NULL;

SELECT id2 || false FROM t2;
SELECT id2 && true FROM t2;

SELECT id2 || 0::boolean FROM t2;
SELECT id2 && 1::boolean FROM t2;

SELECT id2 or false FROM t2;
SELECT id2 and true FROM t2;

SELECT id2 or 0::boolean FROM t2;
SELECT id2 and 1::boolean FROM t2;

drop table t2;
set sql_mode to default;

-- like
CREATE TABLE t2 (id1 enum('aaa','bbb'), id2 enum('ccc','ddd'));
insert into t2 values('aaa', 'ccc');
insert into t2 values('aaa', 'ddd');
insert into t2 values('bbb', 'ddd');
insert into t2 values('bbb', 'ccc');

set SQL_MODE=default;
select * from t2 where id1 like 'aaa'; 
select id1 from t2 where id1 like 1; 
select id1 from t2 where id1 like '1'; 
select id1 from t2 where id1 like '1aaa';
select id1 from t2 where id1 like 'aaa1';
select id1 from t2 where id1 like '#1';
select id1 from t2 where id1 like '1#';
select id1 from t2 where id1 like '.1';
select id1 from t2 where id1 like '1.';
select id1 from t2 where id1 like '%1%';
select id1 from t2 where id1 like '1_3';
select id1 from t2 where id1 like '%a%';
select id1 from t2 where id1 like 'a_';
select id1 from t2 where id1 like 'aa_';
select id1 from t2 where id1 like '_a_';
select id1 from t2 where id1 not like '%a%';
select id1 from t2 where id1 not like 'aa_';
select id1 from t2 where id1 not like '_a_';
set SQL_MODE='';
select * from t2 where id1 like 'aaa'; 
select id1 from t2 where id1 like 1; 
select id1 from t2 where id1 like '1'; 
select id1 from t2 where id1 like '1aaa';
select id1 from t2 where id1 like 'aaa1';
select id1 from t2 where id1 like '#1';
select id1 from t2 where id1 like '1#';
select id1 from t2 where id1 like '.1';
select id1 from t2 where id1 like '1.';
select id1 from t2 where id1 like '%1%';
select id1 from t2 where id1 like '1_3';
select id1 from t2 where id1 like '%a%';
select id1 from t2 where id1 like 'a_';
select id1 from t2 where id1 like 'aa_';
select id1 from t2 where id1 like '_a_';
select id1 from t2 where id1 not like '%a%';
select id1 from t2 where id1 not like 'aa_';
select id1 from t2 where id1 not like '_a_';
set SQL_MODE=default;
drop table t2;

--btree and hash index
CREATE TABLE idx_1 (idx enum('a','b'));
create index idx1 on idx_1 using btree (idx);
insert into idx_1 values ('a'),('b'),('c');
vacuum full analyze idx_1;
explain (COSTS OFF) select * from idx_1 where idx < 'a';
explain (COSTS OFF) select * from idx_1 where idx > 'a';
explain (COSTS OFF) select * from idx_1 where idx = 'a';
explain (COSTS OFF) select * from idx_1 where idx <= 'a';
explain (COSTS OFF) select * from idx_1 where idx >= 'a';

create table idx_2 (idx enum('a','b'));
create index idx2 on idx_2 using hash (idx);
vacuum full analyze idx_2;
insert into idx_2 values ('a'),('b'),('c');
explain (COSTS OFF) select * from idx_2 where idx = 'a';
explain (COSTS OFF) select * from idx_2 where idx > 'a';
drop table idx_1,idx_2;

create table tb1 (id1 enum('a','b', 'c'));
insert into tb1 values ('a'),('b'),('c');
create index idx1 on tb1 using hash(id1);
vacuum full analyze tb1;
set enable_bitmapscan =false;
set enable_seqscan =false;
explain (COSTS OFF) select * from tb1 where id1 = 'a';
set enable_bitmapscan =true;
set enable_seqscan =true;
drop index idx1;

create index idx_tree on tb1(id1);
vacuum full analyze tb1;
set enable_bitmapscan =false;
set enable_seqscan =false;
explain (COSTS OFF) select * from tb1 where id1 < 'a';
explain (COSTS OFF) select * from tb1 where id1 <= 'a';
explain (COSTS OFF) select * from tb1 where id1 = 'a';
explain (COSTS OFF) select * from tb1 where id1 != 'a';
explain (COSTS OFF) select * from tb1 where id1 >= 'a';
explain (COSTS OFF) select * from tb1 where id1 > 'a';
set enable_seqscan =true;
set enable_bitmapscan =true;
drop procedure proc1;
drop table tb1;

CREATE TABLE t2(m1 enum('a','b','z','y','aa'));
INSERT INTO t2 VALUES ('a'),('a'),('b'),('aa'),('z'),('aa'),('z'), (NULL);
SELECT * FROM t2;
SELECT COUNT(*) AS total_rows, MIN(m1) AS min_value, MAX(m1) AS max_value FROM t2;
SELECT AVG(m1) AS avg_value FROM t2;
SELECT SUM(m1) AS sum_value FROM t2;

DROP TABLE t2;

-----------------------------------
show sql_mode;
CREATE TABLE test (
    age INT,
    myname anonymous_enum('a','b')
);
CREATE TABLE test (
    age INT,
    myname ttanonymous_enum('a','b')
);
CREATE TABLE test (
    age INT,
    myname ttanonymous_enumtt('a','b')
);

-- when drop a column using an auto created enum type, the created enum type will also be dropped.
CREATE TABLE test_drop_col (
    age INT,
    myname enum('a','b')
);

SELECT typname FROM pg_type WHERE typname like '%test_drop_col%';
-- ALTER TABLE test DROP myname;
SELECT typname FROM pg_type WHERE typname like '%test_drop_col%';
DROP TABLE test_drop_col;

-- when drop a table using an auto created enum type, the created enum type will also be dropped.
CREATE TABLE test_drop_table (
    age INT,
    myname enum('a','b')
);
SELECT COUNT(*) FROM pg_type WHERE typname like '%anonymous_enum%';
DROP TABLE test_drop_table;
SELECT COUNT(*) FROM pg_type WHERE typname like '%anonymous_enum%';

-- when drop a column using an hand created enum type, the created enum type will not be dropped.
CREATE TYPE job AS enum('x','y');
CREATE TABLE test (age INT, myjob job);
SELECT typname FROM pg_type WHERE typname = 'job';
ALTER TABLE test DROP myjob;
SELECT typname FROM pg_type WHERE typname = 'job';
DROP TABLE test;

-- when alter column type to others, the created enum type will not be dropped.
CREATE TABLE test (age INT, myjob enum('x','y'));
SELECT COUNT(*) FROM pg_type WHERE typname like '%anonymous_enum%';

insert into test values(1, 'x');
insert into test values(2, 'y');
insert into test values(4, 'y');

alter table test alter myjob type text;
SELECT COUNT(*) FROM pg_type WHERE typname like '%anonymous_enum%';

DROP TABLE test;


-- rename a type to a type contain 'anonymous_enum' is not allowed
create type test as enum('a','b');
ALTER type test RENAME TO anonymous_enum;
drop type test;
drop type anonymous_enum;
-- make name more the 64

CREATE TABLE testtttttttttttttttttttttttttttttttttt (age INT, myjobbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb enum('x','y'));

SELECT count(*) FROM pg_type WHERE typname like '%_anonymous_enum%';
drop table testtttttttttttttttttttttttttttttttttt;

-- when drop a table using an hand created enum type, the created enum type will not be dropped.
CREATE TABLE test ( age INT, myjob job);
SELECT typname FROM pg_type WHERE typname = 'job';
DROP TABLE test;
SELECT typname FROM pg_type WHERE typname = 'job';
DROP TYPE job;

-- test function
drop table if exists t1;
create table t1(c_id int, c_name varchar(20), c_age enum('20', '30'));

insert into t1 values(1, 'aa', '30');

create or replace procedure p_enum1()
as
$$
declare
begin 
    for i in 1..3 loop
        insert into t1 values(i, 'name', '20');
    end loop;
end;
$$ language plmysql;

call p_enum1();
select * from t1;
drop table if exists t1;

-- not allowed to create a type with type name contained 'anonymous_enum'
CREATE TYPE country_anonymous_enum_1 AS enum('CHINA','USA');

-- insert with existing index
CREATE TABLE test (age INT, myname enum('a','b','c'));
INSERT INTO test (myname) VALUES (1);

-- insert with index 0 resulting in a null value
INSERT INTO test (myname) VALUES (0);

-- insert with out-of-range index
INSERT INTO test (myname) VALUES (4);

-- query with existing index
SELECT * FROM test WHERE myname=1;

-- query with index 0
SELECT * FROM test WHERE myname=0;

-- query with out-of-range index
SELECT * FROM test WHERE myname=4;

-- delete with existing index
DELETE FROM test WHERE myname=1;

-- delete with index 0
DELETE FROM test WHERE myname=0;

-- delete with out-of-range index
DELETE FROM test WHERE myname=4;
DROP TABLE test;

drop TYPE country_anonymous_enum_1;

-- foreigh key is not allowed
drop table if exists t_enum_0011_01;
CREATE TABLE t_enum_0011_01
(
    W_CITY varchar(60) PRIMARY KEY,
    W_ADDRESS TEXT
);

 CREATE TABLE t_enum_0011_02
(
    W_WAREHOUSE_SK INTEGER NOT NULL,
    W_WAREHOUSE_ID CHAR(16) NOT NULL,
    W_WAREHOUSE_NAME VARCHAR(20) ,
    W_WAREHOUSE_SQ_FT INTEGER ,
    W_STREET_NUMBER CHAR(10) ,
    W_STREET_NAME VARCHAR(60) ,
    W_STREET_TYPE CHAR(15) ,
    W_SUITE_NUMBER CHAR(10) ,
    W_CITY ENUM('xian','gansu','yanan') REFERENCES t_enum_0011_01(W_CITY),
    W_COUNTY VARCHAR(30) ,
    W_STATE CHAR(2) ,
    W_ZIP CHAR(10) ,
    W_COUNTRY VARCHAR(20) ,
    W_GMT_OFFSET DECIMAL(5,2)
);
drop table if exists t_enum_0011_01;
drop table if exists t_enum_0011_02;

drop table if exists chameleon_case6;
create table chameleon_case6(c1 enum('男','女','无'));
insert into chameleon_case6 values('女'), ('男');
select * from chameleon_case6 order by 1;
update chameleon_case6 set c1 ='无' where c1 = '女';
select * from chameleon_case6 order by 1;
drop table if exists chameleon_case6;

drop table if exists test_abc;
create table test_abc (c1 enum('c', 'b', 'a'));
insert into test_abc values('a'), ('b'), ('c');
select c1,
c1 = 'b' AS "c1 = b", c1 <> 'b' AS "c1 <> b", c1 > 'b' AS "c1 > b", c1 >= 'b' AS "c1 >= b", c1 < 'b' AS "c1 < b", c1 <= 'b' AS "c1 <= b",
'b' = c1 AS "b = c1", 'b' <> c1 AS "b <> c1", 'b' > c1 AS "b > c1", 'b' >= c1 AS "b >= c1", 'b' < c1 AS "b < c1", 'b' <= c1 AS "b <= c1"
from test_abc order by 1;
drop table if exists test_abc;

create table enum_test_table(a enum('a', 'b', 'c'));
insert into enum_test_table values('a'), ('b'), ('c');
select * from enum_test_table where a = 1;
select * from enum_test_table where a = 2;
select * from enum_test_table where a = 3;
select * from enum_test_table where 1 = a;
select * from enum_test_table where 2 = a;
select * from enum_test_table where 3 = a;
select * from enum_test_table where 1 > a;
select * from enum_test_table where 2 > a;
select * from enum_test_table where 3 > a;
select * from enum_test_table where 1 >= a;
select * from enum_test_table where 2 >= a;
select * from enum_test_table where 3 >= a;
select * from enum_test_table where 1 < a;
select * from enum_test_table where 2 < a;
select * from enum_test_table where 3 < a;
select * from enum_test_table where 1 <= a;
select * from enum_test_table where 2 <= a;
select * from enum_test_table where 3 <= a;
select * from enum_test_table where 1 != a;
select * from enum_test_table where 2 != a;
select * from enum_test_table where 3 != a;

select a + 1 from enum_test_table;
select a - 1 from enum_test_table;
select a * 1 from enum_test_table;
select a / 1 from enum_test_table;

drop table enum_test_table;

create table test_enum_d(ssl_type enum('any','X509','SPECIFIED') not null default '');
create table test_enum_d(ssl_type enum('','any','X509','SPECIFIED') not null default '');
SELECT enumsortorder,enumlabel FROM pg_enum;
\d test_enum_d
\d+ test_enum_d
drop table test_enum_d;
SELECT enumsortorder,enumlabel FROM pg_enum;

-- test anonymous enum backup
create table student (
     id int not null ,
     name varchar(100),
     gender enum('male', 'female'),
     non_enum enum(),
     primary key(id)
);
insert into student(id, name,gender) values(1, 'bill','male');
select * from student;
\d student
SELECT typname FROM pg_type WHERE typname like '%student%';
create table student_bak as select * from student;
select * from student_bak;
\d student_bak
SELECT typname FROM pg_type WHERE typname like '%student%';

alter table student drop column gender;
select * from student_bak;
select * from student;
\d student_bak
\d student
alter table student drop column non_enum;
select * from student_bak;
select * from student;
\d student_bak
\d student
SELECT enumsortorder,enumlabel FROM pg_enum;

drop table student;
SELECT typname FROM pg_type WHERE typname like '%student%';
SELECT enumsortorder,enumlabel FROM pg_enum;
select * from student_bak;

\d student_bak
drop table student_bak;
SELECT typname FROM pg_type WHERE typname like '%student%';
SELECT enumsortorder,enumlabel FROM pg_enum;

-- test anonymous enum backup :drop table
create table student (
     id int not null ,
     name varchar(100),
     gender enum('male', 'female'),
     non_enum enum(),
     primary key(id)
);
insert into student(id, name,gender) values(1, 'bill','male');
select * from student;
SELECT typname FROM pg_type WHERE typname like '%student%';
SELECT enumsortorder,enumlabel FROM pg_enum;
create table student_bak as select * from student;
select * from student_bak;
SELECT typname FROM pg_type WHERE typname like '%student%';
SELECT enumsortorder,enumlabel FROM pg_enum;
drop table student;
SELECT typname FROM pg_type WHERE typname like '%student%';
SELECT enumsortorder,enumlabel FROM pg_enum;
select * from student_bak;
\d student_bak
alter table student_bak drop column gender;
select * from student_bak;
\d student_bak
SELECT typname FROM pg_type WHERE typname like '%student%';
SELECT enumsortorder,enumlabel FROM pg_enum;
drop table student_bak;
SELECT typname FROM pg_type WHERE typname like '%student%';
SELECT enumsortorder,enumlabel FROM pg_enum;

-- test charset
create table t_charset_enum_column(c enum('a', 'b', 'c') charset test_character_set);
drop table t_charset_enum_column;

-- test alter drop add col 
create table t_enum_column_drop(c1 int, c2 enum('a', 'b'), c3 enum('a', 'b'));
SELECT typname FROM pg_type WHERE typname like '%t_enum_column_drop%';
alter table t_enum_column_drop drop c2;
SELECT typname FROM pg_type WHERE typname like '%t_enum_column_drop%';
drop table t_enum_column_drop;
SELECT typname FROM pg_type WHERE typname like '%t_enum_column_drop%';

create table t_enum_column_alter_type(c1 int, c2 enum('a', 'b'), c3 enum('c', 'd'));
\d t_enum_column_alter_type
SELECT typname FROM pg_type WHERE typname like '%t_enum_column_alter_type%';
insert into t_enum_column_alter_type values(11, 'a', 'c');
select * from t_enum_column_alter_type;
insert into t_enum_column_alter_type values(22, 'yyyy', 'c');----err
select * from t_enum_column_alter_type;
drop table t_enum_column_alter_type;
SELECT typname FROM pg_type WHERE typname like '%t_enum_column_alter_type%';

create table t_enum_column_alter_type(c1 int, c2 enum('a', 'b'), c3 enum('c', 'd'));
\d t_enum_column_alter_type
SELECT typname FROM pg_type WHERE typname like '%t_enum_column_alter_type%';
insert into t_enum_column_alter_type values(11, 'a', 'c');
alter table t_enum_column_alter_type alter c2 type text;
\d t_enum_column_alter_type
SELECT typname FROM pg_type WHERE typname like '%t_enum_column_alter_type%';
insert into t_enum_column_alter_type values(22, 'xxxx', 'c');
select * from t_enum_column_alter_type;
insert into t_enum_column_alter_type values(22, 'yyyy', 'c');
select * from t_enum_column_alter_type;
alter table t_enum_column_alter_type alter c2 type enum('aaaa', 'bbbb'); -- err 'xxxx' not cast to enum
\d t_enum_column_alter_type
insert into t_enum_column_alter_type values(33, 'aaaa', 'c');
select * from t_enum_column_alter_type;
drop table t_enum_column_alter_type;
SELECT typname FROM pg_type WHERE typname like '%t_enum_column_alter_type%';

create table t_enum_column_alter_type(c1 int, c2 enum('a', 'b'), c3 enum('c', 'd'));
\d t_enum_column_alter_type
SELECT typname FROM pg_type WHERE typname like '%t_enum_column_alter_type%';
insert into t_enum_column_alter_type values(11, 'a', 'c');
alter table t_enum_column_alter_type alter c2 type enum('aaaa', 'bbbb'); -- enum to enum
\d t_enum_column_alter_type
insert into t_enum_column_alter_type values(33, 'aaaa', 'c');
select * from t_enum_column_alter_type;
SELECT typname FROM pg_type WHERE typname like '%t_enum_column_alter_type%';
drop table t_enum_column_alter_type;
SELECT typname FROM pg_type WHERE typname like '%t_enum_column_alter_type%';

create table t_enum_column_alter_type(c1 int, c2 enum('a', 'b'), c3 enum('c', 'd'));
\d t_enum_column_alter_type
SELECT typname FROM pg_type WHERE typname like '%t_enum_column_alter_type%';
insert into t_enum_column_alter_type values(11, 'a', 'c');
insert into t_enum_column_alter_type values(33, 'bbbb', 'c');
alter table t_enum_column_alter_type alter c2 type enum('a', 'bbbb'); -- enum to enum
\d t_enum_column_alter_type
insert into t_enum_column_alter_type values(33, 'a', 'c');
insert into t_enum_column_alter_type values(33, 'bbbb', 'c');
select * from t_enum_column_alter_type;
SELECT typname FROM pg_type WHERE typname like '%t_enum_column_alter_type%';
drop table t_enum_column_alter_type;
SELECT typname FROM pg_type WHERE typname like '%t_enum_column_alter_type%';

create table t_enum_column_alter_type(c1 int, c2 enum('a', 'b'), c3 enum('c', 'd'));
\d t_enum_column_alter_type
SELECT typname FROM pg_type WHERE typname like '%t_enum_column_alter_type%';
insert into t_enum_column_alter_type values(11, 'a', 'c');
alter table t_enum_column_alter_type add c4 enum('xxxx', 'yyyy');
\d t_enum_column_alter_type
insert into t_enum_column_alter_type values(33, 'a', 'c', 'yyyy');
select * from t_enum_column_alter_type;
insert into t_enum_column_alter_type values(33, 'aaaa', 'c', 'yyyy');
select * from t_enum_column_alter_type;
SELECT typname FROM pg_type WHERE typname like '%t_enum_column_alter_type%';
alter table t_enum_column_alter_type drop c4;
select * from t_enum_column_alter_type;
\d t_enum_column_alter_type
SELECT typname FROM pg_type WHERE typname like '%t_enum_column_alter_type%';
alter table t_enum_column_alter_type add c4 enum('xxxx', 'yyyy'), add c5 enum('xxxx', 'yyyy'), add c6 enum('xxxx', 'yyyy');
select * from t_enum_column_alter_type;
\d t_enum_column_alter_type
SELECT typname FROM pg_type WHERE typname like '%t_enum_column_alter_type%';
drop table t_enum_column_alter_type;
SELECT typname FROM pg_type WHERE typname like '%t_enum_column_alter_type%';

-- test drop table 
create table t_enum_table_drop(c1 int, c2 enum('aa', 'bb'), c3 enum('aaa', 'bbb'), c4 enum('aaaaa', 'bbbb'));
SELECT typname FROM pg_type WHERE typname like '%t_enum_table_drop%';
drop table t_enum_table_drop;
SELECT typname FROM pg_type WHERE typname like '%t_enum_table_drop%';

-- test alter drop add col --- use pg enum
create type alterenum1 as enum('a','b');
create type alterenum2 as enum('x','y');
create type alterenum3 as enum('ddddd','sssss');
CREATE TABLE test_alter_col (age INT, myjob alterenum1);
insert into test_alter_col values(1, 'a');
alter table test_alter_col alter myjob type alterenum2;
SELECT typname FROM pg_type WHERE typname like '%alterenum%';
alter table test_alter_col alter myjob type text;
SELECT typname FROM pg_type WHERE typname like '%alterenum%';
alter table test_alter_col drop myjob;
SELECT typname FROM pg_type WHERE typname like '%alterenum%';
alter table test_alter_col add c3 alterenum3;
SELECT typname FROM pg_type WHERE typname like '%alterenum%';
select * from test_alter_col;
\d test_alter_col
drop table test_alter_col;
SELECT typname FROM pg_type WHERE typname like '%alterenum%';

drop type alterenum1;
drop type alterenum2;
drop type alterenum3;
--
set SQL_MODE=default;
create type alterenum as enum('a','b');
CREATE TABLE test_alter_col (age INT, myjob enum('x','y'));
SELECT typname FROM pg_type WHERE typname like '%test_alter_col%';
insert into test_alter_col values(1, 'x');
insert into test_alter_col values(2, 'y');
insert into test_alter_col values(4, 'y');
alter table test_alter_col alter myjob type alterenum;
drop table test_alter_col;
drop type alterenum;

set SQL_MODE='';
create type alterenum as enum('a','b');
CREATE TABLE test_alter_col (age INT, myjob enum('x','y'));
SELECT typname FROM pg_type WHERE typname like '%test_alter_col%';
insert into test_alter_col values(1, 'x');
insert into test_alter_col values(2, 'y');
insert into test_alter_col values(4, 'y');
alter table test_alter_col alter myjob type alterenum;
drop table test_alter_col;
drop type alterenum;

set SQL_MODE=default;
create table t_drop_view(c1 int, c2 enum('a'));
create view my_view as select * from t_drop_view;
drop view my_view;
drop table t_drop_view;

set SQL_MODE=default;
drop table if exists test;
create table test(color enum('red', 'green', 'blue', 'purple', 'yellow'));
insert into test values('red');
insert into test values('green');
insert into test values('orange');
insert into test values(0);
insert into test values('0');
insert into test values(1);
insert into test values('1');
insert into test values(1.2);
insert into test values('1.2');
insert into test values(1.4);
insert into test values('1.4');
insert into test values(1.5);
insert into test values('1.5');
insert into test values(1.6);
insert into test values('1.6');
insert into test values('a1');
insert into test values('1a');
insert into test values('a123');
insert into test values('123a');
select * from test;
delete from test;

set SQL_MODE='';
insert into test values('red');
insert into test values('green');
insert into test values('orange');
insert into test values(0);
insert into test values('0');
insert into test values(1);
insert into test values('1');
insert into test values(1.2);
insert into test values('1.2');
insert into test values(1.4);
insert into test values('1.4');
insert into test values(1.5);
insert into test values('1.5');
insert into test values(1.6);
insert into test values('1.6');
insert into test values('a1');
insert into test values('1a');
insert into test values('a123');
insert into test values('123a');
select * from test;
drop table test cascade;

--
--test tailing blank for enum value
--
--expect failure with with duplicate values
set SQL_MODE=default;
--create table tabenum1(a enum ('a','a ') );
create table tabenum2(a enum (' a','a') );
--drop table tabenum1;
drop table tabenum2;
set SQL_MODE='';
--create table tabenum1(a enum ('a','a ') );
create table tabenum2(a enum (' a','a') );
--drop table tabenum1;
drop table tabenum2;

set SQL_MODE=default;
--expect success
create table tabenum(a enum ('a ',' a'));
drop table tabenum;

--expect success
--CREATE TYPE typenum AS ENUM ( 'a', 'a ');
--drop TYPE typenum;
--expect success
CREATE TYPE typenum AS ENUM ( 'a', ' a');
DROP TYPE typenum;

set SQL_MODE=default;
--
-- create enum functio
--
CREATE TABLE staff ( name VARCHAR(40), gender ENUM('male', 'female'), age ENUM('young', 'old'));
select count(*) from pg_type where typname like '%insert_data1%';

-- test function insert enum
CREATE OR REPLACE function insert_data1(param1 CHAR(20), param2 ENUM('male', 'female'), param3 ENUM('young', 'old'))
    RETURNS void AS
$$
DECLARE
BEGIN
INSERT INTO staff (name, gender, age) VALUES (param1,param2, param3);
END
$$ LANGUAGE PLMYSQL;
select count(*) from pg_type where typname like '%insert_data1%';
select insert_data1('aaa', 'yyy', 'xxx'); -- should fail
select insert_data1('aaa', 'female', 'old');
drop function insert_data1;
select * from staff;
select count(*) from pg_type where typname like '%insert_data1%';

-- test function drop 
CREATE OR REPLACE function insert_data1(param1 CHAR(20), param2 ENUM('male', 'female'), param3 ENUM('young', 'old'))
    RETURNS void AS
$$
DECLARE
BEGIN
INSERT INTO staff (name, gender, age) VALUES (param1,param2, param3);
END
$$ LANGUAGE PLMYSQL;
select count(*) from pg_type where typname like '%insert_data1%';
drop function insert_data1;
select count(*) from pg_type where typname like '%insert_data1%';

-- test function drop 
CREATE OR REPLACE function insert_data1(CHAR(20), ENUM('male', 'female'), ENUM('young', 'old'))
    RETURNS void AS
$$
DECLARE
BEGIN
INSERT INTO staff (name, gender, age) VALUES ($1, $2, $3);
END
$$ LANGUAGE PLMYSQL;
select insert_data1('bbb', 'male', 'old');
select * from staff;
drop function insert_data1;
select count(*) from pg_type where typname like '%insert_data1%';
drop table staff;

--test add drop param
CREATE OR REPLACE function insert_data1(CHAR(20), ENUM('male', 'female'))
    RETURNS void AS
$$
DECLARE
BEGIN
END
$$ LANGUAGE PLMYSQL;
select typname from pg_type where typname like '%insert_data1%';
select enumsortorder,enumlabel from pg_enum where enumtypid in (select oid from pg_type where typname like '%insert_data1%');

CREATE OR REPLACE function insert_data1(CHAR(20), ENUM('aaa', 'bbbb'))
    RETURNS void AS
$$
DECLARE
BEGIN
END
$$ LANGUAGE PLMYSQL;
select typname from pg_type where typname like '%insert_data1%';
select enumsortorder,enumlabel from pg_enum where enumtypid in (select oid from pg_type where typname like '%insert_data1%');
select enumsortorder,enumlabel from pg_enum where enumtypid in (select oid from pg_type where typname like '%insert_data1%');

CREATE OR REPLACE function insert_data1(CHAR(20), ENUM('aaa', 'bbbb'), ENUM('zzzz', 'yyyy'))
    RETURNS void AS
$$
DECLARE
BEGIN
END
$$ LANGUAGE PLMYSQL;
select typname from pg_type where typname like '%insert_data1%';
select enumsortorder,enumlabel from pg_enum where enumtypid in (select oid from pg_type where typname like '%insert_data1%');
select enumsortorder,enumlabel from pg_enum where enumtypid in (select oid from pg_type where typname like '%insert_data1%');


CREATE OR REPLACE function insert_data1(CHAR(20))
    RETURNS void AS
$$
DECLARE
BEGIN
END
$$ LANGUAGE PLMYSQL;
select typname from pg_type where typname like '%insert_data1%';
select enumsortorder,enumlabel from pg_enum where enumtypid in (select oid from pg_type where typname like '%insert_data1%');
select enumsortorder,enumlabel from pg_enum where enumtypid in (select oid from pg_type where typname like '%insert_data1%');
select enumsortorder,enumlabel from pg_enum where enumtypid in (select oid from pg_type where typname like '%insert_data1%');

select proname,pronamespace,prorettype,pronargs from pg_proc where proname like '%insert_data1%';
\df insert_data1
drop function insert_data1;
drop function insert_data1(character);
drop function insert_data1(character, insert_data1__anonymous_enum);
drop function insert_data1(character, insert_data1__anonymous_enum1);
drop function insert_data1(character, insert_data1__anonymous_enum2, insert_data1__anonymous_enum3);
select typname from pg_type where typname like '%insert_data1%';

set SQL_MODE='';
--
-- create enum functio
--
CREATE TABLE staff (
    name VARCHAR(40),
    gender ENUM('male', 'female'),
    age ENUM('young', 'old')
);

CREATE OR REPLACE function insert_data2(param1 CHAR(20), param2 ENUM('male', 'female'), param3 ENUM('young', 'old'))
    RETURNS void AS
$$
DECLARE
BEGIN
INSERT INTO staff (name, gender, age) VALUES (param1,param2, param3);
END
$$ LANGUAGE PLMYSQL;

select insert_data2('aaa', 'yyy', 'xxx'); -- should fail
select insert_data2('aaa', 'female', 'old');
drop function insert_data2;
select * from staff;

CREATE OR REPLACE function insert_data2(param1 CHAR(20), param2 ENUM('male', 'female'), param3 ENUM('young', 'old'))
    RETURNS void AS
$$
DECLARE
BEGIN
INSERT INTO staff (name, gender, age) VALUES (param1,param2, param3);
END
$$ LANGUAGE PLMYSQL;
drop function insert_data2;

CREATE OR REPLACE function insert_data2(CHAR(20), ENUM('male', 'female'), ENUM('young', 'old'))
    RETURNS void AS
$$
DECLARE
BEGIN
INSERT INTO staff (name, gender, age) VALUES ($1, $2, $3);
END
$$ LANGUAGE PLMYSQL;
select insert_data2('bbb', 'male', 'old');
select * from staff;

drop function insert_data2;
select count(*) from pg_type where typname like '%insert_data2%';
drop table staff;

-- test expr
create table test (id enum('a', 'b'||'c'));

set sql_mode = default;

--pg err
CREATE TYPE typenum1 AS ENUM ( 'a', 'b');
CREATE TYPE typenum2 AS ENUM ( 'zzzz', 'yyyy');
CREATE OR REPLACE function func1(CHAR(20), typenum1)
    RETURNS void AS
$$
DECLARE
BEGIN
END
$$ LANGUAGE PLMYSQL;

CREATE OR REPLACE function func1(CHAR(20), typenum2)
    RETURNS void AS
$$
DECLARE
BEGIN
END
$$ LANGUAGE PLMYSQL;

select func1('aa'::char(20), 'a');
\df func1

drop function func1(CHAR(20), typenum1);
drop function func1(CHAR(20), typenum2);

set SQL_MODE=default;
-- test cast
create table t_enum(id0 enum('a', 'b', 'c', 'd', 'e', 'f'));
create table t_cast(flag int, id1 int2, id2 int4, id3 int8, id4 float4, id5 float8, id6 int1 unsigned, id7 int2 unsigned, id8 int3 unsigned, id9 int4 unsigned, id10 int8 unsigned);
insert into t_cast values(100, 2, 2, 2, 2.3, 2.6, 4, 4, 4, 4, 4);
insert into t_cast values(200, -1, -1, -1, 2.3, 2.6, 255, 255, 65535, 4294967295, 18446744073709551615);
insert into t_cast values(300, 32767, 2, 9223372036854775807, 2.3, 2.6, 255, 255, 65535, 4294967295, 18446744073709551615);
select * from t_cast;

insert into t_enum (id0) select id1 from t_cast where flag = 100;
insert into t_enum (id0) select id2 from t_cast where flag = 100;
insert into t_enum (id0) select id3 from t_cast where flag = 100;
insert into t_enum (id0) select id4 from t_cast where flag = 100;
insert into t_enum (id0) select id5 from t_cast where flag = 100;
insert into t_enum (id0) select id6 from t_cast where flag = 100;
insert into t_enum (id0) select id7 from t_cast where flag = 100;
insert into t_enum (id0) select id8 from t_cast where flag = 100;
insert into t_enum (id0) select id9 from t_cast where flag = 100;
insert into t_enum (id0) select id10 from t_cast where flag = 100;
select * from t_enum;

insert into t_enum (id0) select id1 from t_cast where flag = 200;
insert into t_enum (id0) select id2 from t_cast where flag = 200;
insert into t_enum (id0) select id3 from t_cast where flag = 200;
insert into t_enum (id0) select id4 from t_cast where flag = 200;
insert into t_enum (id0) select id5 from t_cast where flag = 200;
insert into t_enum (id0) select id6 from t_cast where flag = 200;
insert into t_enum (id0) select id7 from t_cast where flag = 200;
insert into t_enum (id0) select id8 from t_cast where flag = 200;
insert into t_enum (id0) select id9 from t_cast where flag = 200;
insert into t_enum (id0) select id10 from t_cast where flag = 200;
select * from t_enum;

insert into t_enum (id0) select id1 from t_cast where flag = 300;
insert into t_enum (id0) select id2 from t_cast where flag = 300;
insert into t_enum (id0) select id3 from t_cast where flag = 300;
insert into t_enum (id0) select id4 from t_cast where flag = 300;
insert into t_enum (id0) select id5 from t_cast where flag = 300;
insert into t_enum (id0) select id6 from t_cast where flag = 300;
insert into t_enum (id0) select id7 from t_cast where flag = 300;
insert into t_enum (id0) select id8 from t_cast where flag = 300;
insert into t_enum (id0) select id9 from t_cast where flag = 300;
insert into t_enum (id0) select id10 from t_cast where flag = 300;
select * from t_enum;

drop table t_enum, t_cast;

create table t_enum(id0 enum('a', 'b', 'c', 'd', 'e', 'f'));
create table t_cast(flag int, id1 numeric, id2 text, id3 bool);
insert into t_cast values(100, 2.2, '2', true);
insert into t_cast values(200, 2.2, 'e', true);
select * from t_cast;

insert into t_enum (id0) select id1 from t_cast where flag = 100;
insert into t_enum (id0) select id2 from t_cast where flag = 100;
insert into t_enum (id0) select id3 from t_cast where flag = 100;
select * from t_enum;

insert into t_enum (id0) select id1 from t_cast where flag = 200;
insert into t_enum (id0) select id2 from t_cast where flag = 200;
insert into t_enum (id0) select id3 from t_cast where flag = 200;
select * from t_enum;

drop table t_enum, t_cast;

create type pp_colors as enum ('green', 'blue', 'black');
create table pp_enumpart (a pp_colors) partition by list (a);
create table pp_enumpart_green partition of pp_enumpart for values in ('green');
create table pp_enumpart_blue partition of pp_enumpart for values in ('blue');
explain (costs off) select * from pp_enumpart where a = 'blue';
explain (costs off) select * from pp_enumpart where a = 'green';
explain (costs off) select * from pp_enumpart where a in ('blue', 'green');
explain (costs off) select * from pp_enumpart where a = 'xxx';
drop table pp_enumpart;
drop type pp_colors;


create table table_like_test (a enum('1'));
create table table_like_test_2 (like table_like_test);
\d table_like_test
\d table_like_test_2
drop table table_like_test;
drop table table_like_test_2;
SELECT typname FROM pg_type WHERE typname like '%anonymous_enum%';

create table enum_space (val enum('apple ','banana  ','      pear'));
insert into enum_space values('apple '),('banana');
insert into enum_space values('apple   '),('banana');
select *from enum_space;
drop table enum_space;

create table enum_int_binary_ops ( enum_col enum('a', 'b', 'c', 'd'), sm_int smallint, int_val int, big_int bigint );
insert into enum_int_binary_ops values ('a', 1, 1, 1), ('b', 2, 2, 2), ('c', 3, 3, 3), ('d', 4, 4, 5),
(null, null, null, null);

SELECT enum_col, sm_int, int_val,big_int,
  (sm_int >> enum_col) AS enum_bitrshift_sm,
  (int_val >> enum_col) AS enum_bitrshift_int4,
  (big_int >> enum_col) AS enum_bitrshift_int8
FROM enum_int_binary_ops;
SELECT 
  enum_col, sm_int, int_val,big_int,
  (sm_int << enum_col) AS enum_bitrshift_sm, 
  (int_val << enum_col) AS enum_bitrshift_int4,
  (big_int << enum_col) AS enum_bitrshift_int8
FROM enum_int_binary_ops;
drop table enum_int_binary_ops;

create table enum_aggregate ( status enum('正常', '异常', 'pending', '已删除', '乱码....!@#$%'));
insert into enum_aggregate values('正常'),('异常'),('pending'),('已删除'),('乱码....!@#$%'),('正常'),('异常'),('pending'),(null);
select count(status) as total_count from enum_aggregate;
select count(distinct status) as distinct_count from enum_aggregate;
select min(status) as min_status from enum_aggregate;
select max(status) as max_status from enum_aggregate;
select sum(status) as sum_status from enum_aggregate;
select avg(status) as avg_status from enum_aggregate;
drop table enum_aggregate;

create table users (
  username varchar(50),
  status enum('active','inactive','locked')
);
insert into users (username, status) values('zhangsan', 'active'),
('lisi', 'inactive'),('wangwu', 'locked'),('zhaoliu', 'active');
select  username, status from  users where find_in_set(status, 'active,locked') ;
drop table users;

set SQL_MODE='';
create table t_enum_dump_restore(col_1 enum('apple','香蕉','西瓜','pear'));
insert into t_enum_dump_restore values('');
insert into t_enum_dump_restore values('  ');
insert into t_enum_dump_restore values(0);
insert into t_enum_dump_restore values('0');
insert into t_enum_dump_restore values('0aa');
insert into t_enum_dump_restore values('1');
insert into t_enum_dump_restore values('1aa');
insert into t_enum_dump_restore values(NULL);
select * from t_enum_dump_restore where col_1 = ' ';
select * from t_enum_dump_restore where col_1 = '';
select * from t_enum_dump_restore where col_1 IS NULL;
select * from t_enum_dump_restore where col_1 = 0;
select * from t_enum_dump_restore;
drop table t_enum_dump_restore;
set SQL_MODE=default;
create table t_enum_dump_restore(col_1 enum('apple','香蕉','西瓜','pear'));
insert into t_enum_dump_restore values('');
insert into t_enum_dump_restore values('  ');
insert into t_enum_dump_restore values(0);
insert into t_enum_dump_restore values('0');
insert into t_enum_dump_restore values('0aa');
insert into t_enum_dump_restore values('1');
insert into t_enum_dump_restore values('1aa');
insert into t_enum_dump_restore values(NULL);
select * from t_enum_dump_restore where col_1 = ' ';
select * from t_enum_dump_restore where col_1 = '';
select * from t_enum_dump_restore where col_1 IS NULL;
select * from t_enum_dump_restore where col_1 = 0;
select * from t_enum_dump_restore;
drop table t_enum_dump_restore;