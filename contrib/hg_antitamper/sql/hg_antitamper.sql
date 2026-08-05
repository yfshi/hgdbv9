--==============================================================================================--
-- test check per
--==============================================================================================--
insert into hg_security.hg_global_antitamper(reloid, insertrelhash, deleterelhash) values (111, 111, 111);
insert into hg_security.hg_global_antitamper_backup(reloid, insertrelhash, deleterelhash) values (111, 111, 111);
set role syssso;
insert into hg_security.hg_global_antitamper(reloid, insertrelhash, deleterelhash) values (111, 111, 111);
update hg_security.hg_global_antitamper set deleterelhash=1111 where deleterelhash=111;
delete from hg_security.hg_global_antitamper;
select insertrelhash, deleterelhash from hg_security.hg_global_antitamper;
insert into hg_security.hg_global_antitamper_backup(reloid, insertrelhash, deleterelhash) values (111, 111, 111);
update hg_security.hg_global_antitamper_backup set deleterelhash=1111 where deleterelhash=111;
delete from hg_security.hg_global_antitamper_backup;
select insertrelhash, deleterelhash from hg_security.hg_global_antitamper_backup;
reset role;
insert into hg_security.hg_global_antitamper(reloid, insertrelhash, deleterelhash) values (111, 111, 111);
update hg_security.hg_global_antitamper set deleterelhash=1111 where deleterelhash=111;
delete from hg_security.hg_global_antitamper;
select insertrelhash, deleterelhash from hg_security.hg_global_antitamper;
insert into hg_security.hg_global_antitamper_backup(reloid, insertrelhash, deleterelhash) values (111, 111, 111);
update hg_security.hg_global_antitamper_backup set deleterelhash=1111 where deleterelhash=111;
delete from hg_security.hg_global_antitamper_backup;
select insertrelhash, deleterelhash from hg_security.hg_global_antitamper_backup;
--==============================================================================================--
-- test schema antitamper
--==============================================================================================--
create schema ddd with antitamper;
create table ddd.t1(id int, name text);

--create trigger trigger_func BEFORE insert or update on ddd.t1 for each row execute function hg_security.set_generic_hash();

insert into ddd.t1 values(111, 'aaa');
select *, __hg_hash__ from ddd.t1;

select blocknum,insertrelhash,deleterelhash,sqlcommand from hg_security.hg_global_antitamper order by blocknum;
select hg_security.antitamper_hist_check('ddd','t1');
select hg_security.antitamper_hist_repair('ddd','t1');
select hg_security.antitamper_gchain_check('ddd','t1');
select hg_security.antitamper_gchain_repair('ddd','t1');
do $$
DECLARE
	tableoid oid;
	schemaoid oid;
	histname text;
	result record;
BEGIN
	select oid into schemaoid from pg_namespace 
		where nspname = 'ddd';
	select oid into tableoid from pg_class 
		where relname = 't1' and relnamespace = schemaoid;

	histname := 'hist_'::text || tableoid::text ;
	FOR result IN EXECUTE format('select * from hg_security.%I;',histname) LOOP
		raise notice '%',result;
	END LOOP;
END $$;

insert into ddd.t1 values(222, 'bbb');
select *, __hg_hash__ from ddd.t1;
select blocknum,insertrelhash,deleterelhash,sqlcommand from hg_security.hg_global_antitamper order by blocknum;
select hg_security.antitamper_hist_check('ddd','t1');
select hg_security.antitamper_hist_repair('ddd','t1');
select hg_security.antitamper_gchain_check('ddd','t1');
select hg_security.antitamper_gchain_repair('ddd','t1');
do $$
DECLARE
	tableoid oid;
	schemaoid oid;
	histname text;
	result record;
BEGIN
	select oid into schemaoid from pg_namespace 
		where nspname = 'ddd';
	select oid into tableoid from pg_class 
		where relname = 't1' and relnamespace = schemaoid;

	histname := 'hist_'::text || tableoid::text ;
	FOR result IN EXECUTE format('select * from hg_security.%I;',histname) LOOP
		raise notice '%',result;
	END LOOP;
END $$;

update ddd.t1 set name = 'zzz' where id = 111;
select *, __hg_hash__ from ddd.t1;
select blocknum,insertrelhash,deleterelhash,sqlcommand from hg_security.hg_global_antitamper order by blocknum;
select hg_security.antitamper_hist_check('ddd','t1');
select hg_security.antitamper_hist_repair('ddd','t1');
select hg_security.antitamper_gchain_check('ddd','t1');
select hg_security.antitamper_gchain_repair('ddd','t1');
do $$
DECLARE
	tableoid oid;
	schemaoid oid;
	histname text;
	result record;
BEGIN
	select oid into schemaoid from pg_namespace 
		where nspname = 'ddd';
	select oid into tableoid from pg_class 
		where relname = 't1' and relnamespace = schemaoid;

	histname := 'hist_'::text || tableoid::text ;
	FOR result IN EXECUTE format('select * from hg_security.%I;',histname) LOOP
		raise notice '%',result;
	END LOOP;
END $$;

delete from ddd.t1 where id = 222;
select *, __hg_hash__ from ddd.t1;
select blocknum,insertrelhash,deleterelhash,sqlcommand from hg_security.hg_global_antitamper order by blocknum;
select hg_security.antitamper_hist_check('ddd','t1');
select hg_security.antitamper_hist_repair('ddd','t1');
select hg_security.antitamper_gchain_check('ddd','t1');
select hg_security.antitamper_gchain_repair('ddd','t1');
do $$
DECLARE
	tableoid oid;
	schemaoid oid;
	histname text;
	result record;
BEGIN
	select oid into schemaoid from pg_namespace 
		where nspname = 'ddd';
	select oid into tableoid from pg_class 
		where relname = 't1' and relnamespace = schemaoid;

	histname := 'hist_'::text || tableoid::text ;
	FOR result IN EXECUTE format('select * from hg_security.%I;',histname) LOOP
		raise notice '%',result;
	END LOOP;
END $$;

select hg_security.antitamper_gchain_archive('ddd','t1');
select blocknum,insertrelhash,deleterelhash,sqlcommand from hg_security.hg_global_antitamper order by blocknum;
select blocknum,insertrelhash,deleterelhash,sqlcommand from hg_security.hg_global_antitamper_backup order by blocknum;

-- add del column
alter table ddd.t1 alter id type int8;
alter table ddd.t1 add id2 int;
alter table ddd.t1 drop id;

-- del table check depend
drop table ddd.t1;
\d+ ddd.t1;
\d+ hg_security.ddd_t1_hist
select hg_security.antitamper_hist_check('ddd','t1');
select hg_security.antitamper_hist_repair('ddd','t1');
select hg_security.antitamper_gchain_check('ddd','t1');
select hg_security.antitamper_gchain_repair('ddd','t1');

drop schema ddd ;
--==============================================================================================--
-- test table antitamper
--==============================================================================================--
CREATE TABLE t2 (id int, id2 int8, name text) WITH(antitamper=true);
\d+ hg_security.public_t2_hist

insert into t2 values(111, 222, 'aaa');
select *, __hg_hash__ from t2;
select blocknum,insertrelhash,deleterelhash,sqlcommand from hg_security.hg_global_antitamper order by blocknum;
select hg_security.antitamper_hist_check('public', 't2');
select hg_security.antitamper_hist_repair('public', 't2');
select hg_security.antitamper_gchain_check('public', 't2');
select hg_security.antitamper_gchain_repair('public', 't2');
do $$
DECLARE
	tableoid oid;
	schemaoid oid;
	histname text;
	result record;
BEGIN
	select oid into schemaoid from pg_namespace 
		where nspname = 'public';
	select oid into tableoid from pg_class 
		where relname = 't2' and relnamespace = schemaoid;

	histname := 'hist_'::text || tableoid::text ;
	FOR result IN EXECUTE format('select * from hg_security.%I;',histname) LOOP
		raise notice '%',result;
	END LOOP;
END $$;

insert into t2 values(222, 333, 'bbb');
select *, __hg_hash__ from t2;
select blocknum,insertrelhash,deleterelhash,sqlcommand from hg_security.hg_global_antitamper order by blocknum;
select hg_security.antitamper_hist_check('public', 't2');
select hg_security.antitamper_hist_repair('public', 't2');
select hg_security.antitamper_gchain_check('public', 't2');
select hg_security.antitamper_gchain_repair('public', 't2');
do $$
DECLARE
	tableoid oid;
	schemaoid oid;
	histname text;
	result record;
BEGIN
	select oid into schemaoid from pg_namespace 
		where nspname = 'public';
	select oid into tableoid from pg_class 
		where relname = 't2' and relnamespace = schemaoid;

	histname := 'hist_'::text || tableoid::text ;
	FOR result IN EXECUTE format('select * from hg_security.%I;',histname) LOOP
		raise notice '%',result;
	END LOOP;
END $$;

update t2 set name = 'zzz' where id = 111;
select *, __hg_hash__ from t2;
select blocknum,insertrelhash,deleterelhash,sqlcommand from hg_security.hg_global_antitamper order by blocknum;
select hg_security.antitamper_hist_check('public', 't2');
select hg_security.antitamper_hist_repair('public', 't2');
select hg_security.antitamper_gchain_check('public', 't2');
select hg_security.antitamper_gchain_repair('public', 't2');
do $$
DECLARE
	tableoid oid;
	schemaoid oid;
	histname text;
	result record;
BEGIN
	select oid into schemaoid from pg_namespace 
		where nspname = 'public';
	select oid into tableoid from pg_class 
		where relname = 't2' and relnamespace = schemaoid;

	histname := 'hist_'::text || tableoid::text ;
	FOR result IN EXECUTE format('select * from hg_security.%I;',histname) LOOP
		raise notice '%',result;
	END LOOP;
END $$;

delete from t2 where id = 222;
select *, __hg_hash__ from t2;
select blocknum,insertrelhash,deleterelhash,sqlcommand from hg_security.hg_global_antitamper order by blocknum;
select hg_security.antitamper_hist_check('public', 't2');
select hg_security.antitamper_hist_repair('public', 't2');
select hg_security.antitamper_gchain_check('public', 't2');
select hg_security.antitamper_gchain_repair('public', 't2');
do $$
DECLARE
	tableoid oid;
	schemaoid oid;
	histname text;
	result record;
BEGIN
	select oid into schemaoid from pg_namespace 
		where nspname = 'public';
	select oid into tableoid from pg_class 
		where relname = 't2' and relnamespace = schemaoid;

	histname := 'hist_'::text || tableoid::text ;
	FOR result IN EXECUTE format('select * from hg_security.%I;',histname) LOOP
		raise notice '%',result;
	END LOOP;
END $$;

select hg_security.antitamper_gchain_archive('public','t2');
select blocknum,insertrelhash,deleterelhash,sqlcommand from hg_security.hg_global_antitamper order by blocknum;
select blocknum,insertrelhash,deleterelhash,sqlcommand from hg_security.hg_global_antitamper_backup order by blocknum;

-- add del column
alter table t2 alter id type int8;
alter table t2 add id2 int;
alter table t2 drop id;

drop table t2;
\d+ t2;
\d+ hg_security.public_t2_hist
select hg_security.antitamper_hist_check('public','t2');
select hg_security.antitamper_hist_repair('public','t2');
select hg_security.antitamper_gchain_check('public','t2');
select hg_security.antitamper_gchain_repair('public','t2');

--==============================================================================================--
-- test table partition
--==============================================================================================--
create schema nsp2 with antitamper;
create table nsp2.t3 (a int, b int, primary key (a)) partition by range (a);

create table nsp2.t3part2 partition of nsp2.t3 for values from (0) to (1000) ;
create table nsp2.t3part3 partition of nsp2.t3 for values from (1000) to (2000);
create table nsp2.t3part4 partition of nsp2.t3 for values from (2000) to (3000);

\d+ nsp2.t3
\d+ nsp2.t3part2
\d+ nsp2.t3part3
\d+ nsp2.t3part4

insert into nsp2.t3  values (500,1);
insert into nsp2.t3  values (1500,10);
insert into nsp2.t3  values (1600,10);
insert into nsp2.t3  values (2500,10);
insert into nsp2.t3  values (2600,10);
insert into nsp2.t3  values (2700,10);
insert into nsp2.t3  values (3700,10);
select *, __hg_hash__ from nsp2.t3;
select *, __hg_hash__ from nsp2.t3part2;
select *, __hg_hash__ from nsp2.t3part3;
select *, __hg_hash__ from nsp2.t3part4;
update nsp2.t3 set b = 222 where a = 1600;
select *, __hg_hash__ from nsp2.t3;
select *, __hg_hash__ from nsp2.t3part2;
select *, __hg_hash__ from nsp2.t3part3;
select *, __hg_hash__ from nsp2.t3part4;
delete from nsp2.t3 where b = 222;
select *, __hg_hash__ from nsp2.t3;
select *, __hg_hash__ from nsp2.t3part2;
select *, __hg_hash__ from nsp2.t3part3;
select *, __hg_hash__ from nsp2.t3part4;
select blocknum,insertrelhash,deleterelhash,sqlcommand from hg_security.hg_global_antitamper order by blocknum;
select hg_security.antitamper_hist_check('nsp2', 't3');
select hg_security.antitamper_hist_repair('nsp2', 't3');
select hg_security.antitamper_gchain_check('nsp2', 't3');
select hg_security.antitamper_gchain_repair('nsp2', 't3');

do $$
DECLARE
	tableoid oid;
	schemaoid oid;
	histname text;
	result record;
BEGIN
	select oid into schemaoid from pg_namespace 
		where nspname = 'nsp2';
	select oid into tableoid from pg_class 
		where relname = 't3' and relnamespace = schemaoid;

	histname := 'hist_'::text || tableoid::text ;
	FOR result IN EXECUTE format('select * from hg_security.%I;',histname) LOOP
		raise notice '%',result;
	END LOOP;
END $$;

do $$
DECLARE
	tableoid oid;
	schemaoid oid;
	histname text;
	result record;
BEGIN
	select oid into schemaoid from pg_namespace 
		where nspname = 'nsp2';
	select oid into tableoid from pg_class 
		where relname = 't3part2' and relnamespace = schemaoid;

	histname := 'hist_'::text || tableoid::text ;
	FOR result IN EXECUTE format('select * from hg_security.%I;',histname) LOOP
		raise notice '%',result;
	END LOOP;
END $$;

do $$
DECLARE
	tableoid oid;
	schemaoid oid;
	histname text;
	result record;
BEGIN
	select oid into schemaoid from pg_namespace 
		where nspname = 'nsp2';
	select oid into tableoid from pg_class 
		where relname = 't3part3' and relnamespace = schemaoid;

	histname := 'hist_'::text || tableoid::text ;
	FOR result IN EXECUTE format('select * from hg_security.%I;',histname) LOOP
		raise notice '%',result;
	END LOOP;
END $$;

do $$
DECLARE
	tableoid oid;
	schemaoid oid;
	histname text;
	result record;
BEGIN
	select oid into schemaoid from pg_namespace 
		where nspname = 'nsp2';
	select oid into tableoid from pg_class 
		where relname = 't3part4' and relnamespace = schemaoid;

	histname := 'hist_'::text || tableoid::text ;
	FOR result IN EXECUTE format('select * from hg_security.%I;',histname) LOOP
		raise notice '%',result;
	END LOOP;
END $$;

select hg_security.antitamper_gchain_archive('nsp2','t3');
select blocknum,insertrelhash,deleterelhash,sqlcommand from hg_security.hg_global_antitamper order by blocknum;
select blocknum,insertrelhash,deleterelhash,sqlcommand from hg_security.hg_global_antitamper_backup order by blocknum;

-- add del column
alter table nsp2.t3 alter id type int8;
alter table nsp2.t3 add id2 int;
alter table nsp2.t3 drop id;

drop table nsp2.t3;
\d+ nsp2.t3;
\d+ hg_security.nsp2_t3_hist

-------------------------------------------------------------------------------------------------------
create table nsp2.t3 (a int, b int, primary key (a)) partition by range (a);

create table nsp2.t3part2 (a int, b int, primary key (a));
create table nsp2.t3part3 (a int, b int, primary key (a));
create table nsp2.t3part4 (a int, b int, primary key (a));

ALTER TABLE nsp2.t3 ATTACH PARTITION nsp2.t3part2 FOR VALUES FROM (1) TO (1000);
ALTER TABLE nsp2.t3 ATTACH PARTITION nsp2.t3part3 FOR VALUES FROM (1000) TO (2000);
ALTER TABLE nsp2.t3 ATTACH PARTITION nsp2.t3part4 FOR VALUES FROM (2000) TO (3000);

\d+ nsp2.t3
\d+ nsp2.t3part2
\d+ nsp2.t3part3
\d+ nsp2.t3part4

insert into nsp2.t3  values (500,1);
insert into nsp2.t3  values (1500,10);
insert into nsp2.t3  values (1600,10);
insert into nsp2.t3  values (2500,10);
insert into nsp2.t3  values (2600,10);
insert into nsp2.t3  values (2700,10);
select *, __hg_hash__ from nsp2.t3;
select *, __hg_hash__ from nsp2.t3part2;
select *, __hg_hash__ from nsp2.t3part3;
select *, __hg_hash__ from nsp2.t3part4;
update nsp2.t3 set b = 222 where a = 1600;
select *, __hg_hash__ from nsp2.t3;
select *, __hg_hash__ from nsp2.t3part2;
select *, __hg_hash__ from nsp2.t3part3;
select *, __hg_hash__ from nsp2.t3part4;
delete from nsp2.t3 where b = 222;
select *, __hg_hash__ from nsp2.t3;
select *, __hg_hash__ from nsp2.t3part2;
select *, __hg_hash__ from nsp2.t3part3;
select *, __hg_hash__ from nsp2.t3part4;
select blocknum,insertrelhash,deleterelhash,sqlcommand from hg_security.hg_global_antitamper order by blocknum;
select hg_security.antitamper_hist_check('nsp2', 't3');
select hg_security.antitamper_hist_repair('nsp2', 't3');
select hg_security.antitamper_gchain_check('nsp2', 't3');
select hg_security.antitamper_gchain_repair('nsp2', 't3');

do $$
DECLARE
	tableoid oid;
	schemaoid oid;
	histname text;
	result record;
BEGIN
	select oid into schemaoid from pg_namespace 
		where nspname = 'nsp2';
	select oid into tableoid from pg_class 
		where relname = 't3' and relnamespace = schemaoid;

	histname := 'hist_'::text || tableoid::text ;
	FOR result IN EXECUTE format('select * from hg_security.%I;',histname) LOOP
		raise notice '%',result;
	END LOOP;
END $$;

do $$
DECLARE
	tableoid oid;
	schemaoid oid;
	histname text;
	result record;
BEGIN
	select oid into schemaoid from pg_namespace 
		where nspname = 'nsp2';
	select oid into tableoid from pg_class 
		where relname = 't3part2' and relnamespace = schemaoid;

	histname := 'hist_'::text || tableoid::text ;
	FOR result IN EXECUTE format('select * from hg_security.%I;',histname) LOOP
		raise notice '%',result;
	END LOOP;
END $$;

do $$
DECLARE
	tableoid oid;
	schemaoid oid;
	histname text;
	result record;
BEGIN
	select oid into schemaoid from pg_namespace 
		where nspname = 'nsp2';
	select oid into tableoid from pg_class 
		where relname = 't3part3' and relnamespace = schemaoid;

	histname := 'hist_'::text || tableoid::text ;
	FOR result IN EXECUTE format('select * from hg_security.%I;',histname) LOOP
		raise notice '%',result;
	END LOOP;
END $$;

do $$
DECLARE
	tableoid oid;
	schemaoid oid;
	histname text;
	result record;
BEGIN
	select oid into schemaoid from pg_namespace 
		where nspname = 'nsp2';
	select oid into tableoid from pg_class 
		where relname = 't3part4' and relnamespace = schemaoid;

	histname := 'hist_'::text || tableoid::text ;
	FOR result IN EXECUTE format('select * from hg_security.%I;',histname) LOOP
		raise notice '%',result;
	END LOOP;
END $$;

-- add del column
alter table nsp2.t3 alter id type int8;
alter table nsp2.t3 add id2 int;
alter table nsp2.t3 drop id;

drop table nsp2.t3;
\d+ nsp2.t3;
\d+ hg_security.nsp2_t3_hist
drop schema nsp2;
-------------------------------------------------------------------------------------------------------
create table t4 (a int, b int, primary key (a)) partition by range (a) WITH(antitamper=true);
create table t4part2 partition of t4 for values from (0) to (1000);

\d+ t4
\d+ t4part2

insert into t4 values (500,1);
select *, __hg_hash__ from t4;
select *, __hg_hash__ from t4part2;
update t4 set b = 222 where a = 500;
select *, __hg_hash__ from t4;
select *, __hg_hash__ from t4part2;
delete from t4 where b = 222;
select *, __hg_hash__ from t4;
select *, __hg_hash__ from t4part2;

select blocknum,insertrelhash,deleterelhash,sqlcommand from hg_security.hg_global_antitamper order by blocknum;
select hg_security.antitamper_hist_check('public', 't4');
select hg_security.antitamper_hist_repair('public', 't4');
select hg_security.antitamper_gchain_check('public', 't4');
select hg_security.antitamper_gchain_repair('public', 't4');

do $$
DECLARE
	tableoid oid;
	schemaoid oid;
	histname text;
	result record;
BEGIN
	select oid into schemaoid from pg_namespace 
		where nspname = 'public';
	select oid into tableoid from pg_class 
		where relname = 't4' and relnamespace = schemaoid;

	histname := 'hist_'::text || tableoid::text ;
	FOR result IN EXECUTE format('select * from hg_security.%I;',histname) LOOP
		raise notice '%',result;
	END LOOP;
END $$;

do $$
DECLARE
	tableoid oid;
	schemaoid oid;
	histname text;
	result record;
BEGIN
	select oid into schemaoid from pg_namespace 
		where nspname = 'public';
	select oid into tableoid from pg_class 
		where relname = 't4part2' and relnamespace = schemaoid;

	histname := 'hist_'::text || tableoid::text ;
	FOR result IN EXECUTE format('select * from hg_security.%I;',histname) LOOP
		raise notice '%',result;
	END LOOP;
END $$;

drop table t4;
\d+ t4
\d+ t4part2
-------------------------------------------------------------------------------------------------------
CREATE TABLE t4 (a int, b int, primary key (a)) partition by range (a) WITH(antitamper=true) ;
CREATE TABLE t4part2 (a int, b int, primary key (a)) WITH(antitamper=true) ;
insert into t4 values (500,1);
ALTER TABLE t4 ATTACH PARTITION t4part2 FOR VALUES FROM (1) TO (1000);

\d+ t4
\d+ t4part2

alter table t4 set (antitamper=true);
alter table t4part2 set (antitamper=true);
alter table t4 set (antitamper=false);
alter table t4part2 set (antitamper=false);

insert into t4 values (500,1);
select *, __hg_hash__ from t4;
select *, __hg_hash__ from t4part2;
update t4 set b = 222 where a = 500;
select *, __hg_hash__ from t4;
select *, __hg_hash__ from t4part2;
delete from t4 where b = 222;
select *, __hg_hash__ from t4;
select *, __hg_hash__ from t4part2;
select blocknum,insertrelhash,deleterelhash,sqlcommand from hg_security.hg_global_antitamper order by blocknum;
select hg_security.antitamper_hist_check('public', 't4');
select hg_security.antitamper_hist_repair('public', 't4');
select hg_security.antitamper_gchain_check('public', 't4');
select hg_security.antitamper_gchain_repair('public', 't4');

do $$
DECLARE
	tableoid oid;
	schemaoid oid;
	histname text;
	result record;
BEGIN
	select oid into schemaoid from pg_namespace 
		where nspname = 'public';
	select oid into tableoid from pg_class 
		where relname = 't4' and relnamespace = schemaoid;

	histname := 'hist_'::text || tableoid::text ;
	FOR result IN EXECUTE format('select * from hg_security.%I;',histname) LOOP
		raise notice '%',result;
	END LOOP;
END $$;

do $$
DECLARE
	tableoid oid;
	schemaoid oid;
	histname text;
	result record;
BEGIN
	select oid into schemaoid from pg_namespace 
		where nspname = 'public';
	select oid into tableoid from pg_class 
		where relname = 't4part2' and relnamespace = schemaoid;

	histname := 'hist_'::text || tableoid::text ;
	FOR result IN EXECUTE format('select * from hg_security.%I;',histname) LOOP
		raise notice '%',result;
	END LOOP;
END $$;

drop table t4;
\d+ t4
\d+ t4part2

CREATE TABLE t4 (a int, b int, primary key (a)) partition by range (a) WITH(antitamper=true) ;
CREATE TABLE t4part2 (a int, b int, primary key (a));
ALTER TABLE t4 ATTACH PARTITION t4part2 FOR VALUES FROM (1) TO (1000);
drop table t4;

--==============================================================================================--
-- test table schema alter
--==============================================================================================--
------ alter schema ------------------------------------------------------------------------------
create schema ddd with antitamper;
create table ddd.t1(id int);
insert into ddd.t1 values (500);
\d+ ddd.t1

create schema ddd2;
alter table ddd.t1 set schema ddd2;

do $$
DECLARE
	tableoid oid;
	schemaoid oid;
	histname text;
	result record;
BEGIN
	select oid into schemaoid from pg_namespace 
		where nspname = 'ddd';
	select oid into tableoid from pg_class 
		where relname = 't1' and relnamespace = schemaoid;

	histname := 'hist_'::text || tableoid::text ;
	FOR result IN EXECUTE format('select * from hg_security.%I;',histname) LOOP
		raise notice '%',result;
	END LOOP;
END $$;

create schema ddd3 with antitamper;
alter table ddd.t1 set schema ddd3;
\d+ ddd.t1
\d+ ddd3.t1

do $$
DECLARE
	tableoid oid;
	schemaoid oid;
	histname text;
	result record;
BEGIN
	select oid into schemaoid from pg_namespace 
		where nspname = 'ddd3';
	select oid into tableoid from pg_class 
		where relname = 't1' and relnamespace = schemaoid;

	histname := 'hist_'::text || tableoid::text ;
	FOR result IN EXECUTE format('select * from hg_security.%I;',histname) LOOP
		raise notice '%',result;
	END LOOP;
END $$;

ALTER SCHEMA ddd WITH antitamper;
ALTER SCHEMA ddd3 WITH antitamper;

drop table ddd3.t1;
drop schema ddd;
drop schema ddd2;
drop schema ddd3;
------ rename -----------------------------------------------------------------------------------
create schema ddd with antitamper;
create schema ddd2;

create table ddd.t1(id int);
insert into ddd.t1 values (500);
\d+ ddd.t1
do $$
DECLARE
	tableoid oid;
	schemaoid oid;
	histname text;
	result record;
BEGIN
	select oid into schemaoid from pg_namespace 
		where nspname = 'ddd';
	select oid into tableoid from pg_class 
		where relname = 't1' and relnamespace = schemaoid;

	histname := 'hist_'::text || tableoid::text ;
	FOR result IN EXECUTE format('select * from hg_security.%I;',histname) LOOP
		raise notice '%',result;
	END LOOP;
END $$;

alter table ddd.t1 rename to t1rename;
\d+ ddd.t1rename

do $$
DECLARE
	tableoid oid;
	schemaoid oid;
	histname text;
	result record;
BEGIN
	select oid into schemaoid from pg_namespace 
		where nspname = 'ddd';
	select oid into tableoid from pg_class 
		where relname = 't1rename' and relnamespace = schemaoid;

	histname := 'hist_'::text || tableoid::text ;
	FOR result IN EXECUTE format('select * from hg_security.%I;',histname) LOOP
		raise notice '%',result;
	END LOOP;
END $$;

CREATE TABLE t2 (id int, id2 int8) WITH(antitamper=true);
\d+ t2

do $$
DECLARE
	tableoid oid;
	schemaoid oid;
	histname text;
	result record;
BEGIN
	select oid into schemaoid from pg_namespace 
		where nspname = 'public';
	select oid into tableoid from pg_class 
		where relname = 't2' and relnamespace = schemaoid;

	histname := 'hist_'::text || tableoid::text ;
	FOR result IN EXECUTE format('select * from hg_security.%I;',histname) LOOP
		raise notice '%',result;
	END LOOP;
END $$;

alter table t2 rename to t2rename; 
\d+ t2rename

do $$
DECLARE
	tableoid oid;
	schemaoid oid;
	histname text;
	result record;
BEGIN
	select oid into schemaoid from pg_namespace 
		where nspname = 'public';
	select oid into tableoid from pg_class 
		where relname = 't2rename' and relnamespace = schemaoid;

	histname := 'hist_'::text || tableoid::text ;
	FOR result IN EXECUTE format('select * from hg_security.%I;',histname) LOOP
		raise notice '%',result;
	END LOOP;
END $$;

drop table t2rename;
drop table ddd.t1rename;
drop schema ddd;
drop schema ddd2;

--==============================================================================================--
-- test long name table antitamper
--==============================================================================================--
create schema a123456789b123456789c123456789d1 with antitamper;
create table a123456789b123456789c123456789d1.a123456789b123456789c123456789d1(id int, name text);


insert into a123456789b123456789c123456789d1.a123456789b123456789c123456789d1 values(111, 'aaa');
insert into a123456789b123456789c123456789d1.a123456789b123456789c123456789d1 values(222, 'bbb');
update a123456789b123456789c123456789d1.a123456789b123456789c123456789d1 set name = 'zzz' where id = 111;
delete from a123456789b123456789c123456789d1.a123456789b123456789c123456789d1 where id = 222;
select *, __hg_hash__ from a123456789b123456789c123456789d1.a123456789b123456789c123456789d1;

select blocknum,insertrelhash,deleterelhash,sqlcommand from hg_security.hg_global_antitamper order by blocknum;
select hg_security.antitamper_hist_check('a123456789b123456789c123456789d1','a123456789b123456789c123456789d1');
select hg_security.antitamper_hist_repair('a123456789b123456789c123456789d1','a123456789b123456789c123456789d1');
select hg_security.antitamper_gchain_check('a123456789b123456789c123456789d1','a123456789b123456789c123456789d1');
select hg_security.antitamper_gchain_repair('a123456789b123456789c123456789d1','a123456789b123456789c123456789d1');

select hg_security.antitamper_gchain_archive('a123456789b123456789c123456789d1','a123456789b123456789c123456789d1');
select blocknum,insertrelhash,deleterelhash,sqlcommand from hg_security.hg_global_antitamper order by blocknum;
select blocknum,insertrelhash,deleterelhash,sqlcommand from hg_security.hg_global_antitamper_backup order by blocknum;

select * from hg_security.a123456789b123456789c123456789d1_a123456789b123456789c123456789d1_hist;

do $$
DECLARE
	tableoid oid;
	schemaoid oid;
	histname text;
	result record;
BEGIN
	select oid into schemaoid from pg_namespace 
		where nspname = 'a123456789b123456789c123456789d1';
	select oid into tableoid from pg_class 
		where relname = 'a123456789b123456789c123456789d1' and relnamespace = schemaoid;

	histname := 'hist_'::text || tableoid::text ;
	FOR result IN EXECUTE format('select * from hg_security.%I;',histname) LOOP
		raise notice '%',result;
	END LOOP;
END $$;

-- del table check depend
drop table a123456789b123456789c123456789d1.a123456789b123456789c123456789d1;
drop schema a123456789b123456789c123456789d1 ;


--==============================================================================================--
-- test not syssso user
--==============================================================================================--
create user user1;
create schema ddd with antitamper;
create table ddd.t1(id int, name text);

grant usage on schema ddd to user1;
grant all privileges on all tables in schema ddd to user1;
grant usage on schema hg_security to user1;
grant all privileges on all tables in schema hg_security to user1;

insert into ddd.t1 values(111, 'aaa');
insert into ddd.t1 values(222, 'bbb');
update ddd.t1 set name = 'zzz' where id = 111;
delete from ddd.t1 where id = 222;

select *, __hg_hash__ from ddd.t1;
select hg_security.antitamper_hist_check('ddd','t1');
select hg_security.antitamper_hist_repair('ddd','t1');
select hg_security.antitamper_gchain_check('ddd','t1');
select hg_security.antitamper_gchain_repair('ddd','t1');
select hg_security.antitamper_gchain_archive('ddd','t1');

do $$
DECLARE
	tableoid oid;
	schemaoid oid;
	histname text;
	result record;
BEGIN
	select oid into schemaoid from pg_namespace 
		where nspname = 'ddd';
	select oid into tableoid from pg_class 
		where relname = 't1' and relnamespace = schemaoid;

	histname := 'hist_'::text || tableoid::text ;
	FOR result IN EXECUTE format('select * from hg_security.%I;',histname) LOOP
		raise notice '%',result;
	END LOOP;
END $$;

set role user1;
select hg_security.antitamper_hist_check('ddd','t1');
select hg_security.antitamper_hist_repair('ddd','t1');
select hg_security.antitamper_gchain_check('ddd','t1');
select hg_security.antitamper_gchain_repair('ddd','t1');
select hg_security.antitamper_gchain_archive('ddd','t1');
reset role;

drop table ddd.t1;
drop schema ddd ;

revoke all on schema hg_security from user1;
revoke all privileges on all tables in schema hg_security from user1;
drop user user1;

--==============================================================================================--
-- test rowtype
--==============================================================================================--
CREATE TABLE t1 (id int, id2 int8) WITH(antitamper=true);
insert into t1 values(111, 111);

do $$
DECLARE
	v_t1 t1%rowtype;
BEGIN
	select * into v_t1 from t1;
	raise info 'id :=  %, id2 := %, hash = %', v_t1.id , v_t1.id2, v_t1.__hg_hash__;
	select id, id2, __hg_hash__ into v_t1 from t1;
	raise info 'id :=  %, id2 := %, hash = %', v_t1.id , v_t1.id2, v_t1.__hg_hash__;
END $$ LANGUAGE plpgsql;

drop TABLE t1;

CREATE TABLE t2 (id int, id2 int8) ;
do $$
DECLARE
	v_t2 t2%rowtype;
BEGIN
	select * into v_t2 from t2;
	raise info 'id :=  %, id2 := %, hash = %', v_t2.id , v_t2.id2, v_t2.__hg_hash__;
END $$ LANGUAGE plpgsql;
drop TABLE t2;
--==============================================================================================--
-- test antitamper option
--==============================================================================================--
CREATE TABLE t2 (id int, id2 int8) WITH(antitamper=false);
\d+ t2
insert into t2 values(111,222);
alter table t2 set (antitamper=true);
drop table t2;

CREATE TABLE t2 (id int, id2 int8) WITH(antitamper=true);
\d+ t2
insert into t2 values(111,222);
alter table t2 set (antitamper=false);
drop table t2;

--==============================================================================================--
-- test check antitamper column
--==============================================================================================--
create table t(id int) with (antitamper=true);

do $$
declare
r t%rowtype;
begin
r.aaabbb = 1;
end;
$$ language plpgsql;

drop table t;


--==============================================================================================--
-- bug test
--==============================================================================================--
create table public.normal_test (id int, name text);
select hg_security.antitamper_hist_check('public', 'normal_test');
select hg_security.antitamper_hist_repair('public', 'normal_test');
select hg_security.antitamper_gchain_check('public', 'normal_test');
select hg_security.antitamper_gchain_repair('public', 'normal_test');
select hg_security.antitamper_gchain_archive('public','normal_test');
insert into public.normal_test values(1, 'test');
select hg_security.antitamper_hist_check('public', 'normal_test');
select hg_security.antitamper_hist_repair('public', 'normal_test');
select hg_security.antitamper_gchain_check('public', 'normal_test');
select hg_security.antitamper_gchain_repair('public', 'normal_test');
select hg_security.antitamper_gchain_archive('public','normal_test');
drop table public.normal_test;

create unlogged table test (id int, name text) with (antitamper=on);
create temp table test (id int,name text) with (antitamper=on);
create unlogged table test (id int, name text) with (antitamper=off);
create temp table test (id int,name text) with (antitamper=off);

create schema secure_schema with antitamper;
create table secure_schema.test(id int,name text) with (antitamper=off);
create table secure_schema.test(id int,name text) with (antitamper=on , antitamper=on);
create table secure_schema.test(id int,name text) with (antitamper=on , antitamper=off);
create table secure_schema.test(id int,name text) with (antitamper=on);
drop table secure_schema.test;
drop schema secure_schema;

select hg_security.antitamper_hist_check();
select hg_security.antitamper_hist_check('', '');
select hg_security.antitamper_hist_check(null,null);
select hg_security.antitamper_hist_check('secure_schema');

select hg_security.antitamper_hist_repair();
select hg_security.antitamper_hist_repair('', '');
select hg_security.antitamper_hist_repair(null,null);
select hg_security.antitamper_hist_repair('secure_schema');

select hg_security.antitamper_gchain_archive();
select hg_security.antitamper_gchain_archive('', '');
select hg_security.antitamper_gchain_archive(null,null);
select hg_security.antitamper_gchain_archive('secure_schema');

select hg_security.antitamper_gchain_check();
select hg_security.antitamper_gchain_check('', '');
select hg_security.antitamper_gchain_check(null,null);
select hg_security.antitamper_gchain_check('secure_schema');

select hg_security.antitamper_gchain_repair();
select hg_security.antitamper_gchain_repair('', '');
select hg_security.antitamper_gchain_repair(null,null);
select hg_security.antitamper_gchain_repair('secure_schema');

select hg_security.antitamper_gchain_archive();
select hg_security.antitamper_gchain_archive('', '');
select hg_security.antitamper_gchain_archive(null,null);
select hg_security.antitamper_gchain_archive('secure_schema');


create schema secure_schema1 with antitamper;
create schema secure_schema2 with antitamper;
create schema secure_schema3;

create table test(id int, name text);
alter table test set schema secure_schema1;
alter table test set schema secure_schema2;
alter table test set schema secure_schema3;
drop table secure_schema3.test;

create table secure_schema1.test2(id int, name text);
alter table secure_schema1.test2 set schema secure_schema2;
alter table secure_schema2.test2 set schema secure_schema3;
drop table secure_schema2.test2;

drop schema secure_schema1,secure_schema2,secure_schema3;

create schema secure_schema1 with antitamper;
create schema secure_schema2 with antitamper;
create schema secure_schema3;
create table secure_schema1.test1(id int, name text);
create table secure_schema2.test2(id int, name text);
create table secure_schema3.test3(id int, name text);

create table secure_schema1.test4 (a int, b int, primary key (a)) partition by range (a);

create table secure_schema1.test4part2 partition of secure_schema1.test4 for values from (0) to (1000) ;
create table secure_schema1.test4part3 partition of secure_schema1.test4 for values from (1000) to (2000);
create table secure_schema1.test4part4 partition of secure_schema1.test4 for values from (2000) to (3000);

CREATE TABLE test5 (a int, b int, primary key (a)) partition by range (a) WITH(antitamper=true) ;
CREATE TABLE test5part2 (a int, b int, primary key (a)) WITH(antitamper=true) ;
ALTER TABLE test5 ATTACH PARTITION test5part2 FOR VALUES FROM (1) TO (1000);


create table test6 (a int, b int, primary key (a)) partition by range (a) WITH(antitamper=true) ;
create table test6part2 partition of test6 for values from (0) to (1000) ;
create table test6part3 partition of test6 for values from (1000) to (2000);
create table test6part4 partition of test6 for values from (2000) to (3000);

CREATE TABLE public.t1 (id int, id2 int8) WITH(antitamper=1);
CREATE TABLE public.t2 (id int, id2 int8) WITH(antitamper=on);
CREATE TABLE public.t3 (id int, id2 int8) WITH(antitamper=true);
CREATE TABLE public.t4 (id int, id2 int8) WITH(antitamper='on');
CREATE TABLE public.t5 (id int, id2 int8) WITH(antitamper='true');

select relname from hg_security.antitamper_class order by relname;
select nspname,isantitamper from hg_security.antitamper_namespace where nspname like '%secure_schema%' order by nspname;

drop table secure_schema1.test1, secure_schema2.test2, secure_schema3.test3;
drop table secure_schema1.test4;
drop table test5,test6;
drop schema secure_schema1,secure_schema2,secure_schema3;

drop table public.t1;
drop table public.t2;
drop table public.t3;
drop table public.t4;
drop table public.t5;

select relname from hg_security.antitamper_class order by relname;
select nspname,isantitamper from hg_security.antitamper_namespace where nspname like '%secure_schema%' order by nspname;

-- bug:7651
create schema ddd with antitamper;
create table ddd.t1(id int, name text);
create table t2(id int, name text);
create table t3(id int, name text) WITH(antitamper=true);
create table t4(id int, name text) WITH(antitamper=false);

select hg_security.antitamper_hist_check('ddd','t1');
select hg_security.antitamper_hist_check('ddd','t2');
select hg_security.antitamper_hist_check('public','t2');
select hg_security.antitamper_hist_check('public','t3');
select hg_security.antitamper_hist_check('public','t4');

select hg_security.antitamper_hist_repair('ddd','t1');
select hg_security.antitamper_hist_repair('ddd','t2');
select hg_security.antitamper_hist_repair('public','t2');
select hg_security.antitamper_hist_repair('public','t3');
select hg_security.antitamper_hist_repair('public','t4');

select hg_security.antitamper_gchain_check('ddd','t1');
select hg_security.antitamper_gchain_check('ddd','t2');
select hg_security.antitamper_gchain_check('public','t2');
select hg_security.antitamper_gchain_check('public','t3');
select hg_security.antitamper_gchain_check('public','t4');

select hg_security.antitamper_gchain_repair('ddd','t1');
select hg_security.antitamper_gchain_repair('ddd','t2');
select hg_security.antitamper_gchain_repair('public','t2');
select hg_security.antitamper_gchain_repair('public','t3');
select hg_security.antitamper_gchain_repair('public','t4');

select hg_security.antitamper_gchain_archive('ddd','t1');
select hg_security.antitamper_gchain_archive('ddd','t2');
select hg_security.antitamper_gchain_archive('public','t2');
select hg_security.antitamper_gchain_archive('public','t3');
select hg_security.antitamper_gchain_archive('public','t4');

drop table t2, t3, t4;
drop table ddd.t1;
drop schema ddd;
----------------------------------------------------------------------
select 1 from hg_security.antitamper_block;
drop EXTENSION hg_antitamper;
select 1 from hg_security.antitamper_block;
create EXTENSION hg_antitamper;
select 1 from hg_security.antitamper_block;
--==============================================================================================--
-- test se_feature.mode =on/off
--==============================================================================================--
create schema secure_schema with antitamper;
create table  secure_schema.test (id int, name text);
insert into secure_schema.test values(1, 'alex'), (2, 'bob'), (3, 'peter');

create user user11_sso;
create user normal_user;
create user antitamper_spuser with superuser password 'Hello@123';
--------------------------------------------------------------------------
grant usage on schema secure_schema to syssso;
grant all privileges on all tables in schema secure_schema to syssso;
grant usage on schema hg_security to syssso;
grant all privileges on all tables in schema hg_security to syssso;

grant usage on schema secure_schema to user11_sso;
grant all privileges on all tables in schema secure_schema to user11_sso;
grant usage on schema hg_security to user11_sso;
grant all privileges on all tables in schema hg_security to user11_sso;

grant usage on schema secure_schema to normal_user;
grant all privileges on all tables in schema secure_schema to normal_user;
grant usage on schema hg_security to normal_user;
grant all privileges on all tables in schema hg_security to normal_user;
--------------------------------------------------------------------------

grant syssso_role to user11_sso;
-- test se_feature.mode =off
set role syssso;
select hg_security.antitamper_hist_repair('secure_schema', 'test');
select hg_security.antitamper_gchain_repair('secure_schema', 'test');
select hg_security.antitamper_gchain_archive('secure_schema', 'test');

set role user11_sso;
select hg_security.antitamper_hist_repair('secure_schema', 'test');
select hg_security.antitamper_gchain_repair('secure_schema', 'test');
select hg_security.antitamper_gchain_archive('secure_schema', 'test');

set role normal_user;
select hg_security.antitamper_hist_repair('secure_schema', 'test');
select hg_security.antitamper_gchain_repair('secure_schema', 'test');
select hg_security.antitamper_gchain_archive('secure_schema', 'test');

-- check per
\c - syssso
insert into hg_security.hg_global_antitamper(reloid, insertrelhash, deleterelhash) values (111, 111, 111);
update hg_security.hg_global_antitamper set deleterelhash=1111 where deleterelhash=111;
delete from hg_security.hg_global_antitamper;
select insertrelhash, deleterelhash from hg_security.hg_global_antitamper;
insert into hg_security.hg_global_antitamper_backup(reloid, insertrelhash, deleterelhash) values (111, 111, 111);
update hg_security.hg_global_antitamper_backup set deleterelhash=1111 where deleterelhash=111;
delete from hg_security.hg_global_antitamper_backup;
select insertrelhash, deleterelhash from hg_security.hg_global_antitamper_backup;
\c - user11_sso
insert into hg_security.hg_global_antitamper(reloid, insertrelhash, deleterelhash) values (222, 222, 222);
update hg_security.hg_global_antitamper set deleterelhash=2222 where deleterelhash=222;
delete from hg_security.hg_global_antitamper;
select insertrelhash, deleterelhash from hg_security.hg_global_antitamper;
insert into hg_security.hg_global_antitamper_backup(reloid, insertrelhash, deleterelhash) values (222, 222, 222);
update hg_security.hg_global_antitamper_backup set deleterelhash=2222 where deleterelhash=222;
delete from hg_security.hg_global_antitamper_backup;
select insertrelhash, deleterelhash from hg_security.hg_global_antitamper_backup;
\c - normal_user
insert into hg_security.hg_global_antitamper(reloid, insertrelhash, deleterelhash) values (222, 222, 222);
update hg_security.hg_global_antitamper set deleterelhash=2222 where deleterelhash=222;
delete from hg_security.hg_global_antitamper;
select insertrelhash, deleterelhash from hg_security.hg_global_antitamper;
insert into hg_security.hg_global_antitamper_backup(reloid, insertrelhash, deleterelhash) values (222, 222, 222);
update hg_security.hg_global_antitamper_backup set deleterelhash=2222 where deleterelhash=222;
delete from hg_security.hg_global_antitamper_backup;
select insertrelhash, deleterelhash from hg_security.hg_global_antitamper_backup;

\c - antitamper_spuser
insert into hg_security.hg_global_antitamper(reloid, insertrelhash, deleterelhash) values (333, 333, 333);
update hg_security.hg_global_antitamper set deleterelhash=3333 where deleterelhash=333;
select insertrelhash, deleterelhash from hg_security.hg_global_antitamper;
delete from hg_security.hg_global_antitamper;
select insertrelhash, deleterelhash from hg_security.hg_global_antitamper;
insert into hg_security.hg_global_antitamper_backup(reloid, insertrelhash, deleterelhash) values (333, 333, 333);
update hg_security.hg_global_antitamper_backup set deleterelhash=3333 where deleterelhash=333;
select insertrelhash, deleterelhash from hg_security.hg_global_antitamper_backup;
delete from hg_security.hg_global_antitamper_backup;

-- test se_feature.mode =on
\c - antitamper_spuser
select hg_security.antitamper_hist_repair('secure_schema', 'test');
select hg_security.antitamper_gchain_repair('secure_schema', 'test');
select hg_security.antitamper_gchain_archive('secure_schema', 'test');

alter system set se_feature.mode =on;
select pg_reload_conf();

\c - syssso;
select hg_security.antitamper_hist_repair('secure_schema', 'test');
select hg_security.antitamper_gchain_repair('secure_schema', 'test');
select hg_security.antitamper_gchain_archive('secure_schema', 'test');

\c - syssao;
select hg_security.antitamper_hist_repair('secure_schema', 'test');
select hg_security.antitamper_gchain_repair('secure_schema', 'test');
select hg_security.antitamper_gchain_archive('secure_schema', 'test');

\c - user11_sso;
select hg_security.antitamper_hist_repair('secure_schema', 'test');
select hg_security.antitamper_gchain_repair('secure_schema', 'test');
select hg_security.antitamper_gchain_archive('secure_schema', 'test');

\c - normal_user;
select hg_security.antitamper_hist_repair('secure_schema', 'test');
select hg_security.antitamper_gchain_repair('secure_schema', 'test');
select hg_security.antitamper_gchain_archive('secure_schema', 'test');

\c - antitamper_spuser
select hg_security.antitamper_hist_repair('secure_schema', 'test');
select hg_security.antitamper_gchain_repair('secure_schema', 'test');
select hg_security.antitamper_gchain_archive('secure_schema', 'test');

\c - syssso
insert into hg_security.hg_global_antitamper(reloid, insertrelhash, deleterelhash) values (111, 111, 111);
insert into hg_security.hg_global_antitamper_backup(reloid, insertrelhash, deleterelhash) values (111, 111, 111);

\c - normal_user;
insert into hg_security.hg_global_antitamper(reloid, insertrelhash, deleterelhash) values (222, 222, 222);
update hg_security.hg_global_antitamper set deleterelhash=2222 where deleterelhash=222; --error
select insertrelhash, deleterelhash from hg_security.hg_global_antitamper;
delete from hg_security.hg_global_antitamper; --error
select insertrelhash, deleterelhash from hg_security.hg_global_antitamper;

insert into hg_security.hg_global_antitamper_backup(reloid, insertrelhash, deleterelhash) values (222, 222, 222); --error
update hg_security.hg_global_antitamper_backup set deleterelhash=2222 where deleterelhash=222; --error 
select insertrelhash, deleterelhash from hg_security.hg_global_antitamper_backup;
delete from hg_security.hg_global_antitamper_backup; --error
select insertrelhash, deleterelhash from hg_security.hg_global_antitamper_backup;

\c - antitamper_spuser
insert into hg_security.hg_global_antitamper(reloid, insertrelhash, deleterelhash) values (333, 333, 333);
update hg_security.hg_global_antitamper set deleterelhash=3333 where deleterelhash=333; --error
select insertrelhash, deleterelhash from hg_security.hg_global_antitamper;
delete from hg_security.hg_global_antitamper; --error
select insertrelhash, deleterelhash from hg_security.hg_global_antitamper;

insert into hg_security.hg_global_antitamper_backup(reloid, insertrelhash, deleterelhash) values (333, 333, 333); --error
update hg_security.hg_global_antitamper_backup set deleterelhash=3333 where deleterelhash=333; --error
select insertrelhash, deleterelhash from hg_security.hg_global_antitamper_backup;
delete from hg_security.hg_global_antitamper_backup; --error
select insertrelhash, deleterelhash from hg_security.hg_global_antitamper_backup;

\c - syssso
insert into hg_security.hg_global_antitamper(reloid, insertrelhash, deleterelhash) values (111, 111, 111);
update hg_security.hg_global_antitamper set deleterelhash=1111 where deleterelhash=111;
select insertrelhash, deleterelhash from hg_security.hg_global_antitamper;
delete from hg_security.hg_global_antitamper;
select insertrelhash, deleterelhash from hg_security.hg_global_antitamper;

insert into hg_security.hg_global_antitamper_backup(reloid, insertrelhash, deleterelhash) values (111, 111, 111);
update hg_security.hg_global_antitamper_backup set deleterelhash=1111 where deleterelhash=111;
select insertrelhash, deleterelhash from hg_security.hg_global_antitamper_backup;
delete from hg_security.hg_global_antitamper_backup;
select insertrelhash, deleterelhash from hg_security.hg_global_antitamper_backup;

\c - syssso;
alter system set hg_rolstatus.enable=off;
alter system set se_feature.mode=off;
select pg_reload_conf();

\c - antitamper_spuser
drop table  secure_schema.test;
drop schema secure_schema;

revoke all privileges on all tables in schema hg_security from normal_user;
revoke usage on schema hg_security from normal_user;
revoke all privileges on all tables in schema hg_security from user11_sso;
revoke usage on schema hg_security from user11_sso;
drop user user11_sso;
drop user normal_user;