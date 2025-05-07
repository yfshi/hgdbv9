/* skip test if resgroup is not enabled.*/
select 1=2 OR (select count(*) from pg_resgroup_role) = 0
	AS skip_test \gset
\if :skip_test
\quit
\endif

show pg_resource_manager;
show pg_resource_group_queuing_timeout;

show pg_resource_group_bypass;
show pg_resource_group_bypass_catalog_query;

show pg_resource_group_cpu_limit;
show pg_resource_group_cpu_priority;

show pg_resource_group_queuing_timeout;
show pg_resource_group_move_timeout;

CREATE RESOURCE GROUP rgroup0 WITH (
CONCURRENCY=20, 
CPU_MAX_PERCENT=20, 
MEMORY_LIMIT=250, 
CPU_WEIGHT=500, 
MIN_COST=50, 
DISK_QUOTA='300MB',
IO_LIMIT='pg_default: wbps=1000, rbps=1000, wiops=100, riops=100');

CREATE RESOURCE GROUP rgroup1 WITH (
CONCURRENCY=20, 
CPU_MAX_PERCENT=20, 
MEMORY_LIMIT=250, 
CPU_WEIGHT=500, 
MIN_COST=50, 
IO_LIMIT='pg_default: wbps=1000, rbps=1000, wiops=100, riops=100');

CREATE RESOURCE GROUP rgroup2 WITH (
CONCURRENCY=20, 
CPU_MAX_PERCENT=20, 
MEMORY_LIMIT=250, 
CPU_WEIGHT=500, 
MIN_COST=50);

CREATE RESOURCE GROUP rgroup3 WITH (
CONCURRENCY=20, 
CPU_MAX_PERCENT=20, 
MEMORY_LIMIT=250, 
CPU_WEIGHT=500, 
MIN_COST=50);

CREATE RESOURCE GROUP rgroup4 WITH (
CONCURRENCY=20, 
CPU_MAX_PERCENT=20, 
MEMORY_LIMIT=250, 
CPU_WEIGHT=500, 
MIN_COST=50);

CREATE RESOURCE GROUP rgroup5 WITH (
CONCURRENCY=20, 
CPU_MAX_PERCENT=20, 
MEMORY_LIMIT=250, 
CPU_WEIGHT=500, 
MIN_COST=50);

--rgroup1 rgroup2 rgroup3 rgroup4 rgroup5 + 3

select count(*) from pg_resgroup;

select * from pg_resgroup where oid in(6437, 6438, 6441);

select count(*) from pg_resgroupcapability where resgroupid not in(6437, 6438, 6441);;

select * from pg_resgroupcapability where resgroupid in(6437, 6438, 6441) and rgroleid=0;

\d+ pg_resgroup_config

\d+ pg_resgroup_role

\d+ pg_resgroup_status

--select * from pg_resgroup_config;
select groupname, concurrency, cpu_max_percent, cpu_weight, cpuset, memory_limit, min_cost, io_limit from pg_resgroup_config order by groupname;

select * from pg_resgroup_role where rrrolname not in (select rolname from pg_authid  where oid=10) order by rrrolname;

select * from pg_resgroup_status;

create user hguser01 with password 'Hello@123';
create user hguser02 with password 'Hello@123';
create user hguser03 with password 'Hello@123';

alter role hguser01 resource group rgroup2;

alter role hguser02 resource group rgroup3;

alter role hguser03 resource group rgroup4;

create user hguser04 with password 'Hello@123' resource group rgroup5;

select groupname, concurrency, cpu_max_percent, cpu_weight, cpuset, memory_limit, min_cost, io_limit from pg_resgroup_config order by groupname;

--select * from pg_resgroup_config;

select * from pg_resgroup_role where rrrolname not in (select rolname from pg_authid  where oid=10) order by rrrolname;

select * from pg_resgroup_status;

--select groupid, groupname, num_running, num_queueing, num_queued, num_executed, total_queue_duration from pg_resgroup_status order by groupname;
select groupname, num_running, num_queueing, num_queued, total_queue_duration from pg_resgroup_status order by groupname;

DROP USER hguser01;
DROP USER hguser02;
DROP USER hguser03;
DROP USER hguser04;

DROP RESOURCE GROUP rgroup1;
DROP RESOURCE GROUP rgroup2;
DROP RESOURCE GROUP rgroup3;
DROP RESOURCE GROUP rgroup4;
DROP RESOURCE GROUP rgroup5;

select count(*) from pg_resgroup;
select * from pg_resgroup where oid in(6437, 6438, 6441);
select * from pg_resgroup;

select count(*) from pg_resgroupcapability where resgroupid not in(6437, 6438, 6441);

select * from pg_resgroupcapability where resgroupid in(6437, 6438, 6441) and rgroleid=0;


