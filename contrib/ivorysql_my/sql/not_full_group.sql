-- avoid bit-exact output here because operations may not be bit-exact.
SET extra_float_digits = 0;

SELECT any_value(v) FROM (VALUES (1), (2), (3)) AS v (v);
SELECT any_value(v) FROM (VALUES (NULL)) AS v (v);
SELECT any_value(v) FROM (VALUES (NULL), (1), (2)) AS v (v);
SELECT any_value(v) FROM (VALUES (array['hello', 'world'])) AS v (v);
select any_value(v) filter (where v > 2) from (values (1), (2), (3)) as v (v);

set sql_mode='PIPES_AS_CONCAT';

create temp table p_t1 (
  a int,
  b int,
  c int,
  d int,
  primary key(a,b)
) partition by list(a);
create temp table p_t1_1 partition of p_t1 for values in(1);
create temp table p_t1_2 partition of p_t1 for values in(2);

INSERT INTO p_t1 VALUES(1,2,3,4);
INSERT INTO p_t1 VALUES(1,3,4,5);
INSERT INTO p_t1 VALUES(1,4,5,6);
INSERT INTO p_t1 VALUES(2,2,3,4);
INSERT INTO p_t1 VALUES(2,3,4,5);
INSERT INTO p_t1 VALUES(2,4,5,6);

select * from p_t1 group by a,b ORDER BY a,b,c,d;
drop table p_t1;

CREATE TABLE test_having (a int, b int, c char(8), d char);

INSERT INTO test_having VALUES (0, 1, 'XXXX', 'A');
INSERT INTO test_having VALUES (1, 2, 'AAAA', 'b');
INSERT INTO test_having VALUES (2, 2, 'AAAA', 'c');
INSERT INTO test_having VALUES (3, 3, 'BBBB', 'D');
INSERT INTO test_having VALUES (4, 3, 'BBBB', 'e');
INSERT INTO test_having VALUES (5, 3, 'bbbb', 'F');
INSERT INTO test_having VALUES (6, 4, 'cccc', 'g');
INSERT INTO test_having VALUES (7, 4, 'cccc', 'h');
INSERT INTO test_having VALUES (8, 4, 'CCCC', 'I');
INSERT INTO test_having VALUES (9, 4, 'CCCC', 'j');

-- test parallel plan when group by expression is in target list.
select length(d),* from test_having group by length(d);

--no row
select a, sum(distinct b),b from test_having a1
group by a
having exists (select 1 from test_having c
               where  (a1.b + c.b) = c.b);

--10 rows,mysql not support,but we success
select a,max(b) filter (where b > 0),b from test_having GROUP BY a ORDER BY a;

--42 rows,mysql not support,but we success
select a,b from (values (1),(2)) v(x),
 lateral (select a, b, v.x from test_having group by rollup (a,b)) s ORDER BY a,b;

--10 rows,mysql not support,but we success
select a.b,a.a from test_having a, lateral (select a.b from test_having b)ss
group by a.a,a.b;

--ok 10 rows
SELECT a,COUNT(b),b,c,d FROM test_having GROUP BY a ORDER BY a,b,c,d;

--ok 10 rows
SELECT a,b FROM test_having GROUP BY a ORDER BY a,b;

--2 rows
SELECT 1 AS one,a FROM test_having group by b HAVING a > 1 ORDER BY one,a;

SET max_parallel_workers_per_gather = 8;
SET min_parallel_table_scan_size = 0;
SET min_parallel_index_scan_size = 0;
SET parallel_setup_cost = 0;
SET parallel_tuple_cost = 0;
SET max_parallel_workers = 64;

SELECT a,b FROM test_having GROUP BY rollup(a,b*2) ORDER BY a,b;

--no rows
SELECT 1 AS one,a,count(b) FROM test_having HAVING a > 1;

--no rows
SELECT 1 AS one,a,count(b) FROM test_having where b > 0  HAVING a > 1;

--8 rows
SELECT 1 AS one,a FROM test_having HAVING a > 1 ORDER BY a;

--10 rows
SELECT 1 AS one,a FROM test_having having 1 = 1 ORDER BY a;

--1 row
SELECT 1 AS one,a FROM test_having having count(a) > 1;

--mysql error,but we sucess
SELECT 1 AS one,a FROM test_having GROUP BY GROUPING SETS((),()) having a > 1;

--mysql error,but we ok
SELECT 1 AS one,a FROM test_having GROUP BY GROUPING SETS((b),()) having a > 1;

CREATE TABLE test_stmt_id(id integer,salary NUMERIC(25,2),name text);

INSERT INTO test_stmt_id(id,salary) VALUES(1,0.2);
INSERT INTO test_stmt_id VALUES(1,0.4,'0.4');
INSERT INTO test_stmt_id(id,name) VALUES(2,'2');
INSERT INTO test_stmt_id(id,salary) VALUES(2,2.1);
INSERT INTO test_stmt_id(salary,name) VALUES(3,'3.1');
INSERT INTO test_stmt_id(name) VALUES('3.3');
INSERT INTO test_stmt_id(salary) VALUES(4.3);

--3 rows
SELECT id,salary,name FROM test_stmt_id GROUP BY id ORDER BY id,salary,name;

--1 row
SELECT id,salary,name FROM test_stmt_id GROUP BY id HAVING salary > 0.5 ORDER BY salary;

--3 rows
SELECT id,(SELECT a.salary FROM test_stmt_id a WHERE a.name = b.name ORDER BY a.salary limit 1),name FROM test_stmt_id b
GROUP BY id ORDER BY id;

--mysql doesn't support grouping so error
--but we success
SELECT id,name,salary FROM test_stmt_id GROUP BY GROUPING sets(id),GROUPING sets(id,name)
ORDER BY id,name,salary;

--test join & subquery
CREATE TABLE test_join_table(id integer,salary NUMERIC(25,2),name text);
INSERT INTO test_join_table(id,salary) VALUES(1,0.2);
INSERT INTO test_join_table VALUES(1,0.4,'0.4');
INSERT INTO test_join_table(id,name) VALUES(2,'2');
INSERT INTO test_join_table(id,salary) VALUES(2,2.1);
INSERT INTO test_join_table(salary,name) VALUES(3,'3.1');
INSERT INTO test_join_table(name) VALUES('3.3');
INSERT INTO test_join_table(salary) VALUES(4.3);

SET enable_hashjoin = off;
SET enable_nestloop = off;

--test left join
--3 rows
SELECT a.id AS ids,a.name,a.salary,
 a.*,b.* FROM test_stmt_id a LEFT JOIN
 test_join_table b ON a.id = b.id GROUP BY a.id ORDER BY a.id;

--test subquery
--3 rows
SELECT id,name,salary FROM (SELECT id,name,salary * 2 AS salary FROM test_stmt_id) t
GROUP BY id ORDER BY id,name,salary;

--test right join
--3 rows
SELECT a.id AS ids,a.name,a.salary,
 a.*,b.* FROM test_stmt_id a right JOIN
 test_join_table b ON a.id = b.id GROUP BY a.id
ORDER BY ids;

--test cross join
--3 rows
SELECT a.id AS ids,a.name,a.salary,
 a.*,b.* FROM test_stmt_id a cross JOIN
 test_join_table b GROUP BY a.id
ORDER BY ids;

 --test inner join
 --2 rows
SELECT a.id AS ids,a.name,a.salary,
 a.*,b.* FROM test_stmt_id a inner JOIN
 test_join_table b ON a.id = b.id GROUP BY a.id
ORDER BY ids;

--test
--4 rows
SELECT a.id AS id,a.name,a.salary,b.*
 FROM test_stmt_id a inner JOIN
 test_join_table b ON a.id = b.id GROUP BY a.id,b.name
ORDER BY a.id,b.name,a.salary,b.salary;

 --mysql raise error,but we success
SELECT id,(SELECT a.salary FROM test_stmt_id a  LEFT JOIN test_join_table c
    on a.name = c.name AND a.id = b.id limit 1) mds,name FROM test_stmt_id b
GROUP BY id ORDER BY mds,name;

--union all
--3 rows
SELECT * FROM (SELECT id,name,salary FROM test_stmt_id UNION ALL
SELECT id,name ||'xiexie', salary * 14 FROM test_join_table) a
GROUP BY id ORDER BY id,name,salary;

--3 rows
SELECT * FROM (SELECT DISTINCT id,name,salary FROM test_stmt_id UNION ALL
SELECT DISTINCT id,name ||'xiexie', salary * 14 FROM test_join_table) c
GROUP BY id ORDER BY id,name,salary;

--mysql error,but we success
SELECT DISTINCT ON(salary) id,name,salary FROM test_stmt_id UNION ALL
SELECT DISTINCT ON(salary) id,name ||'xiexie', salary * 14 FROM test_join_table
GROUP BY id ORDER BY id,name,salary;

--with cte
--mysql error,but we success
--3 rows
WITH a AS (SELECT * FROM test_stmt_id)
SELECT id,name,salary FROM a GROUP BY id ORDER BY id,name,salary;

--3 rows
WITH a AS (SELECT a.* FROM test_stmt_id a LEFT JOIN test_join_table b ON a.salary = b.salary)
SELECT id,name,salary FROM a GROUP BY id ORDER BY id,name,salary;

--3 rows
WITH a AS (SELECT a.* FROM test_stmt_id a LEFT JOIN test_join_table b ON a.salary = b.salary)
SELECT id,name || 'sale' AS ids ,salary * 14 AS salarys FROM a GROUP BY id
ORDER BY id,ids,salarys;

CREATE TABLE test_test_xx(a integer,b integer,c integer,d integer);

INSERT INTO test_test_xx VALUES(1,2,3,4);
INSERT INTO test_test_xx VALUES(2,3,4,5);
INSERT INTO test_test_xx VALUES(3,4,5,6);
INSERT INTO test_test_xx VALUES(4,5,6,7);

--4 rows
SELECT a + b,SUM(c + d),a,b,c,d FROM test_test_xx GROUP BY a+b ORDER BY a,b,c,d;

--delete
DELETE FROM test_test_xx;

--test null first
INSERT INTO test_test_xx VALUES(1,NULL,NULL,NULL);
INSERT INTO test_test_xx VALUES(1,2,3,4);
INSERT INTO test_test_xx VALUES(2,2,3,4);
INSERT INTO test_test_xx VALUES(2,NULL,NULL,NULL);

--2 rows
SELECT a,b FROM test_test_xx GROUP BY a ORDER BY a;

CREATE TABLE test_any_rowtype(id integer,rowvar test_having,rowvar1 test_join_table,name text);

INSERT INTO test_any_rowtype VALUES(1,NULL,(1,2,'one'),'one');
INSERT INTO test_any_rowtype VALUES(1,(1,2,'xie',1),NULL, NULL);
INSERT INTO test_any_rowtype VALUES(2,(1,3,'sd',1),(1,4,4),'two');
INSERT INTO test_any_rowtype VALUES(2,NULL,NULL,NULL);
INSERT INTO test_any_rowtype VALUES(3,(1,3,NULL,1),(NULL,NULL,NULL),'only');
INSERT INTO test_any_rowtype VALUES(3,(1,NULL,'sjfdsf',1),(NULL,2,'sdfs'),'tow');
INSERT INTO test_any_rowtype VALUES(4,(NULL,NULL,'sjfdsf',1),(NULL,NULL,'sdfs'),'tow');

--4 rows
SELECT * FROM test_any_rowtype GROUP BY id ORDER BY id;
--1 row
SELECT * FROM test_any_rowtype GROUP BY id HAVING rowvar IS NOT null;

--clean data
DROP TABLE test_any_rowtype;
DROP TABLE test_having;
DROP TABLE test_join_table;
DROP TABLE test_stmt_id;
DROP TABLE test_test_xx;
