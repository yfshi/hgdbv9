--
-- PARALLEL
--

--
-- START: setup some tables and data needed by the tests.
--
set enable_insertparallel = true;

create table names(index int, first_name text, last_name text);
create table names2(index int, first_name text, last_name text);
create table names4(index int, first_name text, last_name text);

insert into names values
    (1, 'albert', 'einstein'),
    (2, 'niels', 'bohr'),
    (3, 'erwin', 'schrodinger'),
    (4, 'leonhard', 'euler'),
    (5, 'stephen', 'hawking'),
    (6, 'isaac', 'newton'),
    (7, 'alan', 'turing'),
    (8, 'richard', 'feynman');

--
-- END: setup some tables and data needed by the tests.
--

begin;

-- encourage use of parallel plans
set parallel_setup_cost=0;
set parallel_tuple_cost=0;
set min_parallel_table_scan_size=0;
set max_parallel_workers_per_gather=4;

create table tenk3 (
    unique1    int4 PRIMARY KEY,
    stringu1    name
);

create table para_insert_p1 (
    unique1    int4 PRIMARY KEY,
    stringu1    name
);

create table para_insert_f1 (
    unique1    int4 REFERENCES para_insert_p1(unique1),
    stringu1    name
);

insert into tenk3(unique1) select * from generate_series(0,9999);

--
-- Test INSERT with underlying query.
-- Set parallel dml safe.
-- (should create plan with parallel SELECT, Gather parent node)
--
explain (costs off) insert into para_insert_p1 select unique1, stringu1 from tenk3;
insert into para_insert_p1 select unique1, stringu1 from tenk3;
-- select some values to verify that the parallel insert worked
select count(*), sum(unique1) from para_insert_p1;
-- verify that the same transaction has been used by all parallel workers
select count(*) from (select distinct cmin,xmin from para_insert_p1) as dt;

--
-- Test INSERT with ordered underlying query.
-- (should create plan with parallel SELECT, GatherMerge parent node)
--
truncate para_insert_p1 cascade;
explain (costs off) insert into para_insert_p1 select unique1, stringu1 from tenk3 order by unique1;
insert into para_insert_p1 select unique1, stringu1 from tenk3 order by unique1;
-- select some values to verify that the parallel insert worked
select count(*), sum(unique1) from para_insert_p1;
-- verify that the same transaction has been used by all parallel workers
select count(*) from (select distinct cmin,xmin from para_insert_p1) as dt;

--
-- Test INSERT with RETURNING clause.
-- (should create plan with parallel SELECT, Gather parent node)
--
create table test_data(a int);
insert into test_data select * from generate_series(1,10);

create table test_data1(like test_data);
explain (costs off) insert into test_data1 select * from test_data where a = 10 returning a as data;
insert into test_data1 select * from test_data where a = 10 returning a as data;

--
-- Test INSERT into a table with a foreign key.
-- (Insert into a table with a foreign key is parallel-restricted,
--  as doing this in a parallel worker would create a new commandId
--  and within a worker this is not currently supported)
--
explain (costs off) insert into para_insert_f1 select unique1, stringu1 from tenk3;
insert into para_insert_f1 select unique1, stringu1 from tenk3;
-- select some values to verify that the insert worked
select count(*), sum(unique1) from para_insert_f1;

--
-- Test INSERT with ON CONFLICT ... DO UPDATE ...
-- (should not create a parallel plan)
--
create table test_conflict_table(id serial primary key, somedata int);
explain (costs off) insert into test_conflict_table(id, somedata) select a, a from test_data;
insert into test_conflict_table(id, somedata) select a, a from test_data;
explain (costs off) insert into test_conflict_table(id, somedata) select a, a from test_data ON CONFLICT(id) DO UPDATE SET somedata = EXCLUDED.somedata + 1;

--
-- Test INSERT with underlying query - and RETURNING (no projection)
-- (should create a parallel plan; parallel SELECT)
--
create table names5 (like names);
explain (costs off) insert into names5 select * from names returning *;

--
-- Test INSERT with underlying ordered query - and RETURNING (no projection)
-- (should create a parallel plan; parallel SELECT)
--
create table names6 (like names);
explain (costs off) insert into names6 select * from names order by last_name returning *;
insert into names6 select * from names order by last_name returning *;

--
-- Test INSERT with underlying ordered query - and RETURNING (with projection)
-- (should create a parallel plan; parallel SELECT)
--
create table names7 (like names);
explain (costs off) insert into names7 select * from names order by last_name returning last_name || ', ' || first_name as last_name_then_first_name;
insert into names7 select * from names order by last_name returning last_name || ', ' || first_name as last_name_then_first_name;


--
-- Test INSERT into temporary table with underlying query.
-- (Insert into a temp table is parallel-restricted;
-- should create a parallel plan; parallel SELECT)
--
create temporary table temp_names (like names);
explain (costs off) insert into temp_names select * from names;
insert into temp_names select * from names;

--
-- Test INSERT into partition with underlying query.
--
create table parttable1 (a int, b name) partition by range (a);
create table parttable1_1 partition of parttable1 for values from (0) to (5000);
create table parttable1_2 partition of parttable1 for values from (5000) to (10000);


explain (costs off) insert into parttable1 select unique1,stringu1 from tenk3;
insert into parttable1 select unique1,stringu1 from tenk3;
select count(*) from parttable1_1;
select count(*) from parttable1_2;

--
-- Clean up anything not created in the transaction
--

drop table names;
drop table names2;
drop table test_data;

set enable_insertparallel = false;

-- BUG#Z6513
create table test_id(id integer) with rowid;
