create extension diskquota;

create schema quotainfo;
set search_path to quotainfo;

CREATE role u1 NOLOGIN;
CREATE role u2 NOLOGIN;
CREATE TABLE b (t text);
ALTER TABLE b OWNER TO u1;
CREATE TABLE b2 (t text);
ALTER TABLE b2 OWNER TO u1;

select hg_security.set_role_quota('u1', '1 MB');
select hg_security.set_schema_quota('quotainfo', '2 MB');
select type,name,quotalimitmb from hg_security.diskquota_config_info;

insert into quotainfo.b select generate_series(1,1000);
select type, name, quotalimitmb, use_size from hg_security.pg_regroup_diskquota;

select hg_security.delete_role_quota('u1');
select hg_security.delete_schema_quota('quotainfo');
select type,name,quotalimitmb from hg_security.diskquota_config_info;

drop table b,b2;
drop schema quotainfo;
drop role u1,u2;

drop extension diskquota;