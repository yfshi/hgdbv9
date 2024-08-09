-- test order by null
create table order_by_null_test(val text);
insert into  order_by_null_test(val) values ('A');
insert into  order_by_null_test(val) values ('B');
insert into  order_by_null_test(val) values ('C');
insert into  order_by_null_test(val) values ('a');
insert into  order_by_null_test(val) values ('b');
insert into  order_by_null_test(val) values ('c');
select * from order_by_null_test order by null;
select * from order_by_null_test order by null, val;
drop table   order_by_null_test;

select 1 from dual order by nlssort();
select 1 from dual order by length(nlssort());
select 1 from dual order by length(nlssort(nlssort()));

-- test order by with pseudo-collation
create table order_by_test(id int, val varchar2(8));
insert into order_by_test values (1, 'A');
insert into order_by_test values (2, 'a');
insert into order_by_test values (3, 'b');
insert into order_by_test values (4, 'B');

set nls_sort = binary;
select * from order_by_test order by val;

set nls_sort = binary_ci;
select * from order_by_test order by val;
reset nls_sort;
drop table order_by_test;

-- test group by with pseudo-collation
create table group_by_test(id int, val varchar2(20));
insert into group_by_test values (1, 'a');
insert into group_by_test values (2, 'A');
insert into group_by_test values (3, 'b');
insert into group_by_test values (4, 'B');
insert into group_by_test values (5, 'c');
insert into group_by_test values (6, 'C');

SET enable_hashagg TO false;

set nls_comp = linguistic;
set nls_sort = binary;
select val, count(*) from group_by_test group by val;
set nls_sort = binary_ci;
select val, count(*) from group_by_test group by val;
reset nls_comp;
reset nls_sort;
reset enable_hashagg;
drop table group_by_test;

-- test case insensitive group by
CREATE TABLE group_by_ci_test_1 (c varchar2(20));
INSERT INTO group_by_ci_test_1 VALUES ('a');
INSERT INTO group_by_ci_test_1 VALUES ('a');
INSERT INTO group_by_ci_test_1 VALUES ('A');
SELECT c collate binary_ci, count(c) FROM group_by_ci_test_1 GROUP BY c collate binary_ci;
drop table group_by_ci_test_1;

CREATE TABLE group_by_ci_test_2 (c varchar2(20));
INSERT INTO group_by_ci_test_2 VALUES ('A');
INSERT INTO group_by_ci_test_2 VALUES ('a');
INSERT INTO group_by_ci_test_2 VALUES ('a');
SELECT c collate binary_ci, count(c) FROM group_by_ci_test_2 GROUP BY c collate binary_ci;
drop table group_by_ci_test_2;

-- oracle char type data will trim space when do group by
CREATE TABLE group_by_trim (c char(4));
insert into group_by_trim values ('a');
insert into group_by_trim values ('A ');
select c, count(c) from group_by_trim group by c;
select c COLLATE binary_ci, count(c) from group_by_trim group by c COLLATE binary_ci;
drop table group_by_trim;

-- test bug in group_by_has_partkey()
SET enable_partitionwise_aggregate TO true;
SET enable_partitionwise_join TO true;
SET max_parallel_workers_per_gather TO 0;
SET enable_incremental_sort TO off;
CREATE TABLE pagg_tab (a int, b int, c text collate binary_ci, d int) PARTITION BY LIST(c collate binary);
CREATE TABLE pagg_tab_p1 PARTITION OF pagg_tab FOR VALUES IN ('a', 'b', 'c', 'd');
CREATE TABLE pagg_tab_p2 PARTITION OF pagg_tab FOR VALUES IN ('e', 'f', 'A');
CREATE TABLE pagg_tab_p3 PARTITION OF pagg_tab FOR VALUES IN ('B', 'C', 'D', 'E');
INSERT INTO pagg_tab SELECT i % 20, i % 30, substr('abcdeABCDE', i % 11, 1), i % 30 FROM generate_series(0, 2999) i;
ANALYZE pagg_tab;

set ivorysql.enable_partitionwise_qual_force_pushdown = off;
-- use PARTITIONWISE_AGGREGATE_FULL partitionwise-aggregate strategy, and can push filter into sub table
EXPLAIN SELECT c collate binary_ci, count(c) FROM pagg_tab GROUP BY c collate binary_ci HAVING avg(d) < 15 ORDER BY 1;
-- but result is error !!!
SELECT c collate binary_ci, count(c) FROM pagg_tab GROUP BY c collate binary_ci HAVING avg(d) < 15 ORDER BY 1;

set ivorysql.enable_partitionwise_qual_force_pushdown = on;
-- use PARTITIONWISE_AGGREGATE_PARTIAL partitionwise-aggregate strategy, and cannot push filter into sub table
EXPLAIN SELECT c collate binary_ci, count(c) FROM pagg_tab GROUP BY c collate binary_ci HAVING avg(d) < 15 ORDER BY 1;
-- result is right
SELECT c collate binary_ci, count(c) FROM pagg_tab GROUP BY c collate binary_ci HAVING avg(d) < 15 ORDER BY 1;
RESET enable_partitionwise_aggregate;
RESET enable_partitionwise_join;
RESET max_parallel_workers_per_gather;
RESET enable_incremental_sort;
RESET ivorysql.enable_partitionwise_qual_force_pushdown;
drop table pagg_tab;

-- test performance optimization  in group_by_has_partkey()
SET enable_partitionwise_aggregate TO true;
SET enable_partitionwise_join TO true;
SET max_parallel_workers_per_gather TO 0;
SET enable_incremental_sort TO off;
CREATE TABLE pagg_tab (a int, b int, c text , d int) PARTITION BY LIST(c);
CREATE TABLE pagg_tab_p1 PARTITION OF pagg_tab FOR VALUES IN ('a', 'b', 'c', 'd');
CREATE TABLE pagg_tab_p2 PARTITION OF pagg_tab FOR VALUES IN ('e', 'f', 'A');
CREATE TABLE pagg_tab_p3 PARTITION OF pagg_tab FOR VALUES IN ('B', 'C', 'D', 'E');
INSERT INTO pagg_tab SELECT i % 20, i % 30, substr('abcdeABCDE', i % 11, 1), i % 30 FROM generate_series(0, 2999) i;
ANALYZE pagg_tab;

set ivorysql.enable_partitionwise_qual_force_pushdown = off;
-- result is right, but cannot use PARTITIONWISE_AGGREGATE_FULL partitionwise-aggregate strategy, poor performance
EXPLAIN SELECT c collate binary, count(c) FROM pagg_tab GROUP BY c collate binary HAVING avg(d) < 15 ORDER BY 1;
SELECT c collate binary, count(c) FROM pagg_tab GROUP BY c collate binary HAVING avg(d) < 15 ORDER BY 1;

-- result is right, and can use PARTITIONWISE_AGGREGATE_FULL partitionwise-aggregate strategy, good performance
set ivorysql.enable_partitionwise_qual_force_pushdown = on;
EXPLAIN SELECT c collate binary, count(c) FROM pagg_tab GROUP BY c collate binary HAVING avg(d) < 15 ORDER BY 1;
SELECT c collate binary, count(c) FROM pagg_tab GROUP BY c collate binary HAVING avg(d) < 15 ORDER BY 1;
RESET enable_partitionwise_aggregate;
RESET enable_partitionwise_join;
RESET max_parallel_workers_per_gather;
RESET enable_incremental_sort;
RESET ivorysql.enable_partitionwise_qual_force_pushdown;
drop table pagg_tab;
