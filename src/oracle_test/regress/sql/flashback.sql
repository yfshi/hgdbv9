-- 1. test flashback table to before drop.
create table foo(a int primary key, b int, c text) with (autovacuum_enabled = false);
insert into foo select generate_series(1,100), random() * 100, 'test123rnd' || string_agg(md5(random()::text || clock_timestamp()::text), '') from generate_series(1, 5000);
create index on foo using btree(b);
CREATE OR REPLACE FUNCTION handle_truncate()
RETURNS TRIGGER AS $$
BEGIN
    RAISE NOTICE 'TRUNCATE trigger';
    RETURN NULL;
END;
$$ LANGUAGE plpgsql;
/
CREATE TRIGGER before_truncate_trigger
BEFORE TRUNCATE ON foo
FOR EACH STATEMENT
EXECUTE FUNCTION handle_truncate();
/

drop table foo;
flashback table foo to before drop; --failed

set ivorysql.enable_recycle_bin = on;
create table foo(a int primary key, b int, c text) with (autovacuum_enabled = false);
insert into foo select generate_series(1,100), random() * 100, 'test123rnd' || string_agg(md5(random()::text || clock_timestamp()::text), '') from generate_series(1, 5000);
create index on foo using btree(b);
CREATE OR REPLACE FUNCTION handle_truncate()
RETURNS TRIGGER AS $$
BEGIN
    RAISE NOTICE 'TRUNCATE trigger';
    RETURN NULL;
END;
$$ LANGUAGE plpgsql;
/
CREATE TRIGGER before_truncate_trigger
BEFORE TRUNCATE ON foo
FOR EACH STATEMENT
EXECUTE FUNCTION handle_truncate();
/

drop table foo;
drop table if exists foo;
select rcyoperation, rcytype from pg_recyclebin order by 1,2;
flashback table foo to before drop;
select a, left(c, 10), length(c) from foo order by a limit 1;  --success

drop table foo;  -- more than once.
flashback table foo to before drop;
drop table foo;
select rcyoperation, rcytype from pg_recyclebin order by 1,2;
flashback table foo to before drop;
select a, left(c, 10), length(c) from foo order by a limit 1;

drop table foo;  -- more than one table.
create table foo(a int primary key, b text, c int) with (autovacuum_enabled = false);
insert into foo select generate_series(200,300), 'test456rnd' || string_agg(md5(random()::text || clock_timestamp()::text), ''), random() * 100 from generate_series(1, 5000);
create table foo1(a int primary key, b text, c int) with (autovacuum_enabled = false);
insert into foo1 select generate_series(200,300), 'test789rnd' || string_agg(md5(random()::text || clock_timestamp()::text), ''), random() * 100 from generate_series(1, 5000);
create index on foo using btree(c);
CREATE OR REPLACE FUNCTION handle_truncate()
RETURNS TRIGGER AS $$
BEGIN
    RAISE NOTICE 'TRUNCATE trigger';
    RETURN NULL;
END;
$$ LANGUAGE plpgsql;
/
CREATE TRIGGER before_truncate_trigger
BEFORE TRUNCATE ON foo
FOR EACH STATEMENT
EXECUTE FUNCTION handle_truncate();
/
select a, left(b, 10), length(b) from foo order by a limit 1;

drop table foo;
drop table foo1;
select rcyoperation, rcytype from pg_recyclebin order by 1,2;
DECLARE     -- more than one table, use rcyname
    v_rcyname text;
    sql_str   text;
BEGIN
    select rcyname into v_rcyname from pg_recyclebin where rcyoriginname = 'foo1' and rcytype = 0;
    EXECUTE format('flashback table foo, %I to before drop;', v_rcyname);
END;
/
select a, left(b, 10), length(b) from foo order by a limit 1;
select a, left(b, 10), length(b) from foo1 order by a limit 1;
drop table foo;
PURGE table foo;  -- purge table
drop table foo1 purge;
select rcyoperation, rcytype from pg_recyclebin order by 1,2;
flashback table foo to before drop;
select a, left(b, 10), length(b) from foo order by a limit 1;

drop table foo purge;  -- drop with purge
select rcyoperation, rcytype from pg_recyclebin order by 1,2;
set ivorysql.enable_recycle_bin = off;

-- 2. test flashback table to before truncate.
create table foo(a int primary key, b int, c text) with (autovacuum_enabled = false);
insert into foo select 123, random() * 100, 'test123rnd' || string_agg(md5(random()::text || clock_timestamp()::text), '') from generate_series(1, 5000);
create index on foo using btree(b);
CREATE OR REPLACE FUNCTION handle_truncate()
RETURNS TRIGGER AS $$
BEGIN
    RAISE NOTICE 'TRUNCATE trigger';
    RETURN NULL;
END;
$$ LANGUAGE plpgsql;
/
CREATE TRIGGER before_truncate_trigger
BEFORE TRUNCATE ON foo
FOR EACH STATEMENT
EXECUTE FUNCTION handle_truncate();
/

truncate foo;
flashback table foo to before truncate; --failed

set ivorysql.enable_recycle_bin = on;
insert into foo select 123, random() * 100, 'test123rnd' || string_agg(md5(random()::text || clock_timestamp()::text), '') from generate_series(1, 5000);
truncate foo; --warning
select rcyoperation, rcytype from pg_recyclebin order by 1,2;
insert into foo select 456, random() * 100, 'test456rnd' || string_agg(md5(random()::text || clock_timestamp()::text), '') from generate_series(1, 5000);
truncate foo; --warning
select rcyoperation, rcytype from pg_recyclebin order by 1,2;
insert into foo select 789, random() * 100, 'test789rnd' || string_agg(md5(random()::text || clock_timestamp()::text), '') from generate_series(1, 5000);
select a, left(c, 10), length(c) from foo;  --success
flashback table foo to before truncate;
select a, left(c, 10), length(c) from foo;  --success
flashback table foo to before truncate;
select a, left(c, 10), length(c) from foo;  --success

truncate foo;  -- more than once.
flashback table foo to before truncate;
truncate foo;
select rcyoperation, rcytype from pg_recyclebin order by 1,2;
flashback table foo to before truncate;
select a, left(c, 10), length(c) from foo;
select rcyoperation, rcytype from pg_recyclebin order by 1,2;

truncate foo; --warning
select rcyoperation, rcytype from pg_recyclebin order by 1,2;
insert into foo select 456, random() * 100, 'test456rnd' || string_agg(md5(random()::text || clock_timestamp()::text), '') from generate_series(1, 5000);
truncate foo; --warning
select rcyoperation, rcytype from pg_recyclebin order by 1,2;
insert into foo select 789, random() * 100, 'test789rnd' || string_agg(md5(random()::text || clock_timestamp()::text), '') from generate_series(1, 5000);
truncate foo; --warning
select rcyoperation, rcytype from pg_recyclebin order by 1,2;

purge table foo;  -- purge table
select rcyoperation, rcytype from pg_recyclebin order by 1,2;
purge table foo;
select rcyoperation, rcytype from pg_recyclebin order by 1,2;
flashback table foo to before truncate;
select a, left(c, 10), length(c) from foo;
set ivorysql.enable_recycle_bin = off;
drop table foo;

-- 3. test purge recyclebin.
set ivorysql.enable_recycle_bin = on;
create table foo(a int primary key, b int, c text) with (autovacuum_enabled = false);
insert into foo select 123, random() * 100, 'test123rnd' || string_agg(md5(random()::text || clock_timestamp()::text), '') from generate_series(1, 5000);
create index on foo using btree(b);
CREATE OR REPLACE FUNCTION handle_truncate()
RETURNS TRIGGER AS $$
BEGIN
    RAISE NOTICE 'TRUNCATE trigger';
    RETURN NULL;
END;
$$ LANGUAGE plpgsql;
/
CREATE TRIGGER before_truncate_trigger
BEFORE TRUNCATE ON foo
FOR EACH STATEMENT
EXECUTE FUNCTION handle_truncate();
/
truncate foo;
drop table foo;
select rcyoperation, rcytype from pg_recyclebin order by 1,2;
purge recyclebin;
select rcyoperation, rcytype from pg_recyclebin order by 1,2;
set ivorysql.enable_recycle_bin = off;

select rcyoperation, rcytype from pg_recyclebin order by 1,2;
