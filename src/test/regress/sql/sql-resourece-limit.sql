--
-- This file used to test SQL resource limits, including memory usage,
-- disk I/O, and CPU limits.
--

--
-- test memory alloc
--

--error
show sql_resource_limit;
show sqlallocated_memory;

set sql_resource_limit to strict;
set sqlallocated_memory to '3MB';

show sql_resource_limit;
show sqlallocated_memory;

create table tb(id int, name text);

insert into tb values (1, 'zhangsan');
insert into tb values (2, 'lisi');
insert into tb values (1, repeat('a', 534288001));	--error
insert into tb select i, 'name' || i from generate_series(1, 1000000) i;
delete from tb;

set sqlallocated_memory to '1MB';
insert into tb values (1, repeat('a', 2));
insert into tb values (2, repeat('a', 10));
insert into tb values (3, repeat('a', 5000));
insert into tb values (4, repeat('a', 13886080));	--error

update tb set name = repeat('a', 104857600);	--error
update tb set name = repeat('a', 104841848);	--error
update tb set name = repeat('a', 10000) where id = 2;
select length(name) from tb;
delete from tb;

set sqlallocated_memory to '2MB';
insert into tb values (1, repeat('a', 200012));
insert into tb values (2, repeat('a', 2097152)); --error
insert into tb values (3, repeat('a', 3145728)); --error
delete from tb;

--waring
set sql_resource_limit to loose;
set sqlallocated_memory to '1MB';
insert into tb values (1, 'zhangsan');
insert into tb values (2, 'lisi');
insert into tb values (1, repeat('a', 1048576));	--waring
--insert into tb select i, 'name' || i from generate_series(1, 1000000) i;
delete from tb;

set sqlallocated_memory to '2MB';
insert into tb values (1, repeat('a', 2097152));	--waring
insert into tb values (2, repeat('a', 2098234));	--waring
insert into tb values (3, repeat('a', 1145728));
update tb set name = repeat('a', 3145728);	--waring
delete from tb;

set sqlallocated_memory to '3MB';
insert into tb values (1, repeat('a', 48576));
insert into tb values (2, repeat('a', 2097152));	--waring
insert into tb values (3, repeat('a', 3145728));	--waring
update tb set name = repeat('a', 3145728) where id = 1;	--waring
delete from tb;

drop table tb;

reset sqlallocated_memory;

--
-- test write of I/O
--
show sqlwrite_buffer;

set sqlwrite_buffer = '1MB';
set sql_resource_limit to strict;

create table tb(id int, name text);
alter table tb alter column name set storage external;
insert into tb values (1, repeat('a', 1000));
insert into tb values (2, repeat('a', 1048576));	--error
insert into tb values (3, repeat('a', 13886080));	--error
insert into tb values (4, repeat('a', 83886080));	--error
update tb set name = repeat('a', 1048576) where id = 1;	--error
update tb set name = repeat('a', 104841848);	--error

delete from tb;

set sqlwrite_buffer = '2MB';
insert into tb values (1, repeat('a', 2097));
insert into tb values (2, repeat('a', 5000));
insert into tb values (3, repeat('a', 104857600));	--error
insert into tb values (4, repeat('a', 13886080));	--error
insert into tb values (5, repeat('a', 41943040));	--error
insert into tb values (6, repeat('a', 52428800));	--error
insert into tb values (7, repeat('a', 2097152));	--error
-- update tb set name = repeat('a', 209715200) where id = 1;	--error
-- update tb set name = repeat('a', 157286400);	--error
-- update tb set name = repeat('a', 83886080);	--error
drop table tb;

reset sqlwrite_buffer;

--
-- test read of I/O
--
show sqlread_buffer;

set sql_resource_limit to strict;
set sqlread_buffer = '1MB';

create table tb(id int, name text);
alter table tb alter column name set storage external;
insert into tb values (1, repeat('a', 100));
insert into tb values (2, repeat('a', 1048576));	--error
insert into tb values (6, repeat('a', 5242880));	--error
insert into tb values (3, repeat('a', 10485760));	--error
insert into tb values (4, repeat('a', 20971520));	--error
insert into tb values (5, repeat('a', 104857600));	--error
update tb set name = repeat('a', 10485760) where id = 1;	--error
update tb set name = repeat('a', 20971520) where id = 2;
delete from tb;

set sqlread_buffer = '2MB';
insert into tb values (1, repeat('a', 100));
insert into tb values (2, repeat('a', 104857600));	--error
update tb set name = repeat('a', 104857600) where id = 1;	--error
delete from tb;

set sqlread_buffer = '3MB';
insert into tb values (1, repeat('a', 1));
insert into tb values (2, repeat('a', 104857600));	--error
update tb set name = repeat('a', 104857600) where id = 1;	--error
select * from tb;
delete from tb;

set sqlread_buffer = '5MB';
insert into tb values (1, repeat('a', 100));
insert into tb values (2, repeat('a', 200));
insert into tb values (3, repeat('a', 300));
insert into tb values (4, repeat('a', 400));
insert into tb values (5, repeat('a', 10485760));	--error
update tb set name = repeat('a', 10485760) where id = 1;	--error
select * from tb;

set sqlread_buffer = '10MB';
insert into tb values (1, repeat('a', 100));
insert into tb values (2, repeat('a', 200));
insert into tb values (3, repeat('a', 300));
insert into tb values (4, repeat('a', 400));
insert into tb values (5, repeat('a', 10485760));	--error
update tb set name = repeat('a', 10485760) where id = 1;	--error
delete from tb;

--warning
set sql_resource_limit to loose;
set sqlread_buffer = '1MB';

insert into tb values (1, repeat('a', 100));
insert into tb values (2, repeat('a', 200));
insert into tb values (3, repeat('a', 300));
insert into tb values (4, repeat('a', 400));
insert into tb values (5, repeat('a', 3145728));	--warning
update tb set name = repeat('a', 3145728) where id = 1;	--warning
delete from tb;

set sqlread_buffer = '2MB';
insert into tb values (1, repeat('a', 100));
insert into tb values (2, repeat('a', 200));
insert into tb values (3, repeat('a', 300));
insert into tb values (4, repeat('a', 500));
insert into tb values (5, repeat('a', 3145728));	--warning
update tb set name = repeat('a', 4194304) where id = 1;	--warning
delete from tb;

set sqlread_buffer = '3MB';
insert into tb values (1, repeat('a', 100));
insert into tb values (2, repeat('a', 200));
insert into tb values (3, repeat('a', 500));
insert into tb values (4, repeat('a', 4194304));	--warning
update tb set name = repeat('a', 5194304) where id = 4;	--warning
delete from tb;

drop table tb;
reset sqlread_buffer;
set sql_resource_limit to off;
