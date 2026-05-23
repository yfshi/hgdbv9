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

create table fb_rename_a(a int);
create table fb_rename_b(a int);
insert into fb_rename_a values (1);
insert into fb_rename_b values (2);
drop table fb_rename_a;
drop table fb_rename_b;
flashback table fb_rename_a, fb_rename_b to before drop rename to fb_rename_a_new, fb_rename_b_new;
select * from fb_rename_a_new;
select * from fb_rename_b_new;
drop table fb_rename_a_new purge;
drop table fb_rename_b_new purge;

create table fb_drop_multi_a(a int);
create table fb_drop_multi_b(a int);
create table fb_drop_multi_c(a int);
insert into fb_drop_multi_a values (1);
insert into fb_drop_multi_b values (2);
insert into fb_drop_multi_c values (3);
drop table fb_drop_multi_a, fb_drop_multi_b, fb_drop_multi_c;
select relname from pg_class
    where relname in ('fb_drop_multi_a', 'fb_drop_multi_b', 'fb_drop_multi_c')
    order by 1;
select rcyoriginname, rcyoperation, rcytype from pg_recyclebin
    where rcyoriginname like 'fb_drop_multi_%'
    order by 1, 2, 3;
flashback table fb_drop_multi_a, fb_drop_multi_b, fb_drop_multi_c to before drop;
select 'a' as rel, * from fb_drop_multi_a;
select 'b' as rel, * from fb_drop_multi_b;
select 'c' as rel, * from fb_drop_multi_c;
drop table fb_drop_multi_a purge;
drop table fb_drop_multi_b purge;
drop table fb_drop_multi_c purge;

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

create table fb_trunc_multi_a(a int);
create table fb_trunc_multi_b(a int);
insert into fb_trunc_multi_a values (1);
insert into fb_trunc_multi_b values (2);
truncate fb_trunc_multi_a, fb_trunc_multi_b;
select rcyoriginname, rcyoperation, rcytype from pg_recyclebin
    where rcyoriginname like 'fb_trunc_multi_%'
    order by 1, 2, 3;
select 'a' as rel, count(*) from fb_trunc_multi_a;
select 'b' as rel, count(*) from fb_trunc_multi_b;
flashback table fb_trunc_multi_a, fb_trunc_multi_b to before truncate;
select 'a' as rel, * from fb_trunc_multi_a;
select 'b' as rel, * from fb_trunc_multi_b;
drop table fb_trunc_multi_a purge;
drop table fb_trunc_multi_b purge;

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
