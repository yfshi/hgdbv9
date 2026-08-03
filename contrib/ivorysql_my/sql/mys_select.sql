--
-- MYSQL SELECT
--

--test dual
select 1 + 1 from dual;
select * from dual;

create table sales (product varchar(50), region varchar(50), amount decimal(10,2));
insert into sales (product, region, amount) values ('ProductA', 'North', 100),('ProductA', 'South', 150),('ProductB', 'North', 200),('ProductB', 'South', 250);
select * from sales union select * from dual;
select * from dual, sales;

--with rollup
select product, region,SUM(amount) AS total_amount from sales group by product, region with rollup;
select product, region,SUM(amount) AS total_amount from sales group by product, region, amount with rollup;

--group by desc/asc
select product, region,SUM(amount) AS total_amount from sales group by product desc, region asc;
select product, region,SUM(amount) AS total_amount from sales group by product desc, region asc order by product asc, region desc;


--having without group by
select * from sales having amount > 100;
select * from sales where product = 'ProductA' having amount > 0;
select sum(amount) from sales having  SUM(amount) > 10;

--view test
create view dual_v as select 1+1 from dual;
select * from dual_v;
create view with_rollup_v as select product, region,SUM(amount) AS total_amount from sales group by product, region with rollup;
select * from with_rollup_v;
create view groupby_v as select product, region,SUM(amount) AS total_amount from sales group by product desc, region asc;
select * from groupby_v;
create view having_v as select * from sales having amount > 100;
select * from having_v;

drop view dual_v;
drop view with_rollup_v;
drop view groupby_v;
drop view having_v;
DROP TABLE sales;

--test POC-huaneng parens fromlist
CREATE TABLE tb1(id int);
CREATE TABLE tb2(name char(20));
CREATE TABLE tb3(id int);
CREATE TABLE tb4(id int, name char(20));
INSERT INTO tb1 VALUES(7355608);
INSERT INTO tb2 VALUES('Nathan Schmitt');
INSERT INTO tb3 VALUES(2),(4);
INSERT INTO tb4 VALUES(1, 'ZywOo');
SELECT * FROM tb1, tb2, tb3, tb4;
SELECT * FROM (tb1, tb2, tb3, tb4);
SELECT * FROM (tb1, tb2), tb3, tb4;
SELECT * FROM tb1, (tb2, tb3, tb4);
SELECT * FROM (tb1, (tb2, tb3)), tb4;
SELECT * FROM ((tb1, tb2, tb3, tb4));
SELECT * FROM (((tb1, tb2)), ((tb3, tb4)));
SELECT * FROM (((tb1 JOIN tb3 ON tb1.id != tb3.id)));   --ok here
SELECT * FROM (tb1);    --syntax error, not support yet
SELECT * FROM (((tb1)));    --syntax error, not support yet
DROP TABLE tb1;
DROP TABLE tb2;
DROP TABLE tb3;
DROP TABLE tb4;

--
-- MYSQL-style extra parentheses in FROM
--

DROP TABLE IF EXISTS mysql_from_parens_test;
DROP TABLE IF EXISTS mysql_from_parens_test1;

CREATE TABLE mysql_from_parens_test (
  c integer
);

CREATE TABLE mysql_from_parens_test1 (
  c integer
);

INSERT INTO mysql_from_parens_test VALUES (11);
INSERT INTO mysql_from_parens_test1 VALUES (22);

select * from ((select (select 1) a)c, (select (select 2) b)d);
SELECT * FROM (((mysql_from_parens_test)));   --syntax error, not support yet

SELECT *
FROM ((mysql_from_parens_test), (mysql_from_parens_test1));     --syntax error, not support yet

SELECT *
FROM (((mysql_from_parens_test), (mysql_from_parens_test1)));     --syntax error, not support yet

SELECT *
FROM ((SELECT (SELECT c FROM mysql_from_parens_test) a) a,
      (SELECT (SELECT c FROM mysql_from_parens_test1) b) b);

SELECT *
FROM (((SELECT 1 AS x) s));         --syntax error, not support yet

SELECT *
FROM (((SELECT 1 AS x)));

SELECT *
FROM (((mysql_from_parens_test JOIN mysql_from_parens_test1 ON true)));

DROP TABLE mysql_from_parens_test;
DROP TABLE mysql_from_parens_test1;


--test single quote alias
create table alias_test (c1 int);
insert into alias_test values (1) returning c1 as 'return c1';
select c1 as 'column 1' from alias_test;
select c1 + 1 as 'result' from alias_test;
select round(c1 + 1) as "round" from alias_test;
select (select 1) as 'subquery', c1 as 'column' from alias_test;
delete from alias_test returning c1 as 'return c1';

drop table alias_test;



--
-- MySQL single-quoted SELECT target aliases
--

DROP TABLE IF EXISTS mysql_select_alias_test;

CREATE TABLE mysql_select_alias_test (
  c integer,
  label text
);

INSERT INTO mysql_select_alias_test VALUES (1, 'one');
INSERT INTO mysql_select_alias_test VALUES (2, 'two');

SELECT c AS 'as alias'
FROM mysql_select_alias_test
ORDER BY c;

SELECT c 'bare alias'
FROM mysql_select_alias_test
ORDER BY c;

SELECT m.c 'bare alias'
FROM mysql_select_alias_test m
ORDER BY c;

-- SELECT c + 10 'expr alias'
-- FROM mysql_select_alias_test
-- ORDER BY c;

SELECT upper(label) AS 'Upper Label'
FROM mysql_select_alias_test
ORDER BY c;

-- SELECT (SELECT c + 100) 'nested alias'
-- FROM mysql_select_alias_test
-- ORDER BY c;

-- SELECT (SELECT label
--         FROM mysql_select_alias_test t2
--         WHERE t2.c = t1.c) 'nested from alias'
-- FROM mysql_select_alias_test t1
-- ORDER BY c;

-- SELECT ((SELECT c 'inner alias'
--          FROM mysql_select_alias_test t2
--          WHERE t2.c = t1.c)) 'outer alias'
-- FROM mysql_select_alias_test t1
-- ORDER BY c;

SELECT 'literal value' 'literal alias';

SELECT CAST('true' AS bool) AS normal_bool_literal;

-- SELECT pg_typeof(now())::text 'current time alias';

-- SELECT length(text 'abc') 'func arg cast';

SELECT CAST('1' AS int) AS int_literal;

SELECT CAST('xy' AS character(2)) AS char_literal;

SELECT CAST('10' AS bit(2)) AS bit_literal;

SELECT CAST('2024-01-02 03:04:05' AS timestamp(2)) AS timestamp_literal;

SELECT CAST('2024-01-02 03:04:05+00' AS timestamptz) AS timestamptz_literal;

SELECT CAST('03:04:05' AS time(2)) AS time_literal;

SELECT CAST('((0,0),(1,0),(0,1))' AS polygon) AS polygon_literal;

SELECT CAST('1:1:' AS txid_snapshot) AS txid_snapshot_literal;

SELECT c AS 'select'
FROM mysql_select_alias_test
ORDER BY c;

DROP TABLE mysql_select_alias_test;

--
-- Merged from column_quote_alias.sql
--

DROP TABLE IF EXISTS t_alias_case0001_1;
CREATE TABLE t_alias_case0001_1 (a int);
INSERT INTO t_alias_case0001_1 (a) VALUES (1);
-- select character varying 'keyword_test' from t_alias_case0001_1;
-- select character 'keyword_test' from t_alias_case0001_1;
-- select nvarchar 'keyword_test' from t_alias_case0001_1;
-- select nvarchar2 'keyword_test' from t_alias_case0001_1;
-- -- select national varchar 'keyword_test' from t_alias_case0001_1;
-- select VARCHAR 'keyword_test' from t_alias_case0001_1;
-- select VARCHAR2 'keyword_test' from t_alias_case0001_1;
-- select NATIONAL CHARACTER 'keyword_test' from t_alias_case0001_1;
-- select NATIONAL CHAR 'keyword_test' from t_alias_case0001_1;
-- select NCHAR 'keyword_test' from t_alias_case0001_1;
-- select NCHAR VARYING 'keyword_test' from t_alias_case0001_1;
-- select CHAR 'keyword_test' from t_alias_case0001_1;
-- select CHAR VARYING 'keyword_test' from t_alias_case0001_1;
-- select TEXT 'keyword_test' from t_alias_case0001_1;
-- select character(2) 'keyword_test' from t_alias_case0001_1;
-- select text(2) 'keyword_test' from t_alias_case0001_1;
-- select char(1) 'keyword_test' from t_alias_case0001_1;
-- select char varying(1) 'keyword_test' from t_alias_case0001_1;
-- select timestamp(2) 'keyword_test' from t_alias_case0001_1;
-- select time(2) 'keyword_test' from t_alias_case0001_1;
-- select smalldatetime 'keyword_test' from t_alias_case0001_1;
-- select int 'keyword_test' from t_alias_case0001_1;
-- select tinyint 'keyword_test' from t_alias_case0001_1;
-- select smallint 'keyword_test' from t_alias_case0001_1;
-- select mediumint 'keyword_test' from t_alias_case0001_1;
-- select bigint 'keyword_test' from t_alias_case0001_1;
-- select real 'keyword_test' from t_alias_case0001_1;
-- select float 'keyword_test' from t_alias_case0001_1;
-- select BINARY_DOUBLE 'keyword_test' from t_alias_case0001_1;
-- select BINARY_INTEGER 'keyword_test' from t_alias_case0001_1;
-- select decimal 'keyword_test' from t_alias_case0001_1;
-- select number 'keyword_test' from t_alias_case0001_1;
-- select dec 'keyword_test' from t_alias_case0001_1;
-- select numeric 'keyword_test' from t_alias_case0001_1;
-- select fixed 'keyword_test' from t_alias_case0001_1;
-- select boolean 'keyword_test' from t_alias_case0001_1;
-- select bit(2) 'keyword_test' from t_alias_case0001_1;
-- select bit varying 'keyword_test' from t_alias_case0001_1;
-- select bit 'keyword_test' from t_alias_case0001_1;
SELECT a as ' ' from t_alias_case0001_1;
-- SELECT a ' ' from t_alias_case0001_1;
SELECT a as '(ss)' from t_alias_case0001_1;
-- SELECT a '(ss)' from t_alias_case0001_1;
SELECT a as '334ss' from t_alias_case0001_1;
-- SELECT a '334ss' from t_alias_case0001_1;
SELECT a as '#' from t_alias_case0001_1;
-- SELECT a '#' from t_alias_case0001_1;
-- SELECT a '$&%^@' from t_alias_case0001_1;
SELECT a as '$&%^@' from t_alias_case0001_1;
SELECT a as '!' from t_alias_case0001_1;
SELECT a as '@' from t_alias_case0001_1;
SELECT a as '$' from t_alias_case0001_1;
SELECT a as '&' from t_alias_case0001_1;
-- SELECT a  '!' from t_alias_case0001_1;
-- SELECT a  '@' from t_alias_case0001_1;
-- SELECT a  '$' from t_alias_case0001_1;
-- SELECT a  '&' from t_alias_case0001_1;
SELECT a as 'abort' from t_alias_case0001_1;
-- SELECT a 'abort' from t_alias_case0001_1;
SELECT a as 'as' from t_alias_case0001_1;
-- SELECT a 'as' from t_alias_case0001_1;
SELECT a as 'all' from t_alias_case0001_1;
-- SELECT a 'all' from t_alias_case0001_1;
SELECT a as 'cast' from t_alias_case0001_1;
-- SELECT a 'cast' from t_alias_case0001_1;
SELECT a as 'BIT' from t_alias_case0001_1;
-- SELECT a 'BIT' from t_alias_case0001_1;
SELECT a as 'BINARY_DOUBLE' from t_alias_case0001_1;
-- SELECT a 'BINARY_DOUBLE' from t_alias_case0001_1;
SELECT a as 'BINARY_INTEGER' from t_alias_case0001_1;
-- SELECT a 'BINARY_INTEGER' from t_alias_case0001_1;
SELECT a as 'BOOLEAN_P' from t_alias_case0001_1;
-- SELECT a 'BOOLEAN_P' from t_alias_case0001_1;
SELECT a as 'kssdjskjcjdfkvndfkvndk ndkfn kldfvklx m dfjcvaldskcdlfk ndfvkaldierufq8239e0d98wjeca,s.c ssss kssdjskjcjdfkvndfkvndk ndkfn kldfvklx m dfjcvaldskcdlfk ndfvkaldierufq8239e0d98wjeca,s.c sssskssdjskjcjdfkvndfkvndk ndkfn kldfvklx m dfjcvaldskcdlfk ndfvkaldierufq8239e0d98wjeca,s.c sssskssdjskjcjdfkvndfkvndk ndkfn kldfvklx m dfjcvaldskcdlfk ndfvkaldierufq8239e0d98wjeca,s.c sssskssdjskjcjdfkvndfkvndk ndkfn kldfvklx m dfjcvaldskcdlfk ndfvkaldierufq8239e0d98wjeca,s.c sssskssdjskjcjdfkvndfkvndk ndkfn kldfvklx m dfjcvaldskcdlfk ndfvkaldierufq8239e0d98wjeca,s.c sssskssdjskjcjdfkvndfkvndk ndkfn kldfvklx m dfjcvaldskcdlfk ndfvkaldierufq8239e0d98wjeca,s.c sssskssdjskjcjdfkvndfkvndk ndkfn kldfvklx m dfjcvaldskcdlfk ndfvkaldierufq8239e0d98wjeca,s.c sssskssdjskjcjdfkvndfkvndk ndkfn kldfvklx m dfjcvaldskcdlfk ndfvkaldierufq8239e0d98wjeca,s.c ssss' from t_alias_case0001_1;
-- SELECT a 'kssdjskjcjdfkvndfkvndk ndkfn kldfvklx m dfjcvaldskcdlfk ndfvkaldierufq8239e0d98wjeca,s.c ssss kssdjskjcjdfkvndfkvndk ndkfn kldfvklx m dfjcvaldskcdlfk ndfvkaldierufq8239e0d98wjeca,s.c sssskssdjskjcjdfkvndfkvndk ndkfn kldfvklx m dfjcvaldskcdlfk ndfvkaldierufq8239e0d98wjeca,s.c sssskssdjskjcjdfkvndfkvndk ndkfn kldfvklx m dfjcvaldskcdlfk ndfvkaldierufq8239e0d98wjeca,s.c sssskssdjskjcjdfkvndfkvndk ndkfn kldfvklx m dfjcvaldskcdlfk ndfvkaldierufq8239e0d98wjeca,s.c sssskssdjskjcjdfkvndfkvndk ndkfn kldfvklx m dfjcvaldskcdlfk ndfvkaldierufq8239e0d98wjeca,s.c sssskssdjskjcjdfkvndfkvndk ndkfn kldfvklx m dfjcvaldskcdlfk ndfvkaldierufq8239e0d98wjeca,s.c sssskssdjskjcjdfkvndfkvndk ndkfn kldfvklx m dfjcvaldskcdlfk ndfvkaldierufq8239e0d98wjeca,s.c sssskssdjskjcjdfkvndfkvndk ndkfn kldfvklx m dfjcvaldskcdlfk ndfvkaldierufq8239e0d98wjeca,s.c ssss' from t_alias_case0001_1;
SELECT a as 'ssss' from t_alias_case0001_1;
-- SELECT a 'ssss' from t_alias_case0001_1;
SELECT a as 'SSSS' from t_alias_case0001_1;
-- SELECT a 'SSSS' from t_alias_case0001_1;
DROP TABLE t_alias_case0001_1;
