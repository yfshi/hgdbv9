create extension diskquota;

-- Test min quota reserve file behavior
-- Create test principals for schema-level and role-level min quota checks.
create schema smin;
create role rminq nologin;

select oid as dbid from pg_database where datname = current_database() \gset
select oid as smin_oid from pg_namespace where nspname = 'smin' \gset
select oid as rmin_oid from pg_roles where rolname = 'rminq' \gset

-- Begin Bug#z8703 Bug#z8704
select hg_security.set_schema_quota('smin', '1 MB', true);
select hg_security.set_role_quota('rminq', '1 MB', true);

select pg_sleep(5);

select size
from pg_stat_file(format('diskquota_min_reserve/nsp_%s_%s.reserve', :'dbid', :'smin_oid'));

select size
from pg_stat_file(format('diskquota_min_reserve/rol_%s_%s.reserve', :'dbid', :'rmin_oid'));

select hg_security.delete_schema_quota('smin');
select hg_security.delete_role_quota('rminq');

select pg_sleep(5);

select size
from pg_stat_file(format('diskquota_min_reserve/nsp_%s_%s.reserve', :'dbid', :'smin_oid'));

select size
from pg_stat_file(format('diskquota_min_reserve/rol_%s_%s.reserve', :'dbid', :'rmin_oid'));
-- End Bug#z8703 Bug#z8704

-- Enable min quota for both schema and role, then verify the metadata views.
select hg_security.set_schema_quota('smin', '4 MB', true);
select hg_security.set_role_quota('rminq', '3 MB', true);

select type, name, quotalimitmb, minlimitmb
from hg_security.diskquota_config_info
where name = 'smin';

select type, name, quotalimitmb, minlimitmb
from hg_security.diskquota_config_info
where name = 'rminq';

-- Wait for the worker to materialize reserve files.
select pg_sleep(5);

-- Reserve files should match the configured min quota before any data is written.
select size
from pg_stat_file(format('diskquota_min_reserve/nsp_%s_%s.reserve', :'dbid', :'smin_oid'));

select size
from pg_stat_file(format('diskquota_min_reserve/rol_%s_%s.reserve', :'dbid', :'rmin_oid'));

create table smin.tmin(v text);
alter table smin.tmin owner to rminq;

-- Insert enough incompressible text to consume more than the reserved quota.
insert into smin.tmin
select (select string_agg(md5((i * 1000 + g)::text), '')
        from generate_series(1, 64) as g)
from generate_series(1, 5000) as i;

-- After usage reaches the min quota, the reserve files should shrink to zero.
select pg_sleep(5);

select size
from pg_stat_file(format('diskquota_min_reserve/nsp_%s_%s.reserve', :'dbid', :'smin_oid'));

select size
from pg_stat_file(format('diskquota_min_reserve/rol_%s_%s.reserve', :'dbid', :'rmin_oid'));

-- Truncating the table should release used space and restore the reserve files.
truncate table smin.tmin;

select pg_sleep(5);

select size
from pg_stat_file(format('diskquota_min_reserve/nsp_%s_%s.reserve', :'dbid', :'smin_oid'));

select size
from pg_stat_file(format('diskquota_min_reserve/rol_%s_%s.reserve', :'dbid', :'rmin_oid'));

-- Lowering min quota should shrink the reserve files to the new target size.
select hg_security.set_schema_quota('smin', '2 MB', true);
select hg_security.set_role_quota('rminq', '1 MB', true);

select pg_sleep(5);

select size
from pg_stat_file(format('diskquota_min_reserve/nsp_%s_%s.reserve', :'dbid', :'smin_oid'));

select size
from pg_stat_file(format('diskquota_min_reserve/rol_%s_%s.reserve', :'dbid', :'rmin_oid'));

-- Disabling min quota should remove the configuration from views and clear reserves.
select hg_security.set_schema_quota('smin', '-1 MB', true);
select hg_security.set_role_quota('rminq', '-1 MB', true);

select hg_security.set_schema_quota('smin', '0 MB', true);
select hg_security.set_role_quota('rminq', '0 MB', true);

select pg_sleep(5);

select size
from pg_stat_file(format('diskquota_min_reserve/nsp_%s_%s.reserve', :'dbid', :'smin_oid'));

select size
from pg_stat_file(format('diskquota_min_reserve/rol_%s_%s.reserve', :'dbid', :'rmin_oid'));

select type, name, quotalimitmb, minlimitmb
from hg_security.diskquota_config_info
where name = 'smin';

select type, name, quotalimitmb, minlimitmb
from hg_security.diskquota_config_info
where name = 'rminq';

select hg_security.delete_schema_quota('smin');
select hg_security.delete_role_quota('rminq');

select type, name, quotalimitmb, minlimitmb
from hg_security.diskquota_config_info
where name = 'smin';

select type, name, quotalimitmb, minlimitmb
from hg_security.diskquota_config_info
where name = 'rminq';

drop table smin.tmin;
drop schema smin;
drop role rminq;

drop extension diskquota;