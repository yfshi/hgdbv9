select cast(1 as time(0));
select cast(20121212 as datetime(0));

select cast(8385959.9999994 as time(5));    -- 838:59:59.00000
select cast(8385959.9999995 as time(5));    -- 838:59:59.00000, warning
select cast(8385959.999999 as time(6));     -- 838:59:59.999999
select cast(8385959.9999994 as time(6));    -- 838:59:59.999999
select cast(8385959.9999995 as time(6));    -- 838:59:59.000000, warning
select cast(8385959.1 as time(5));          -- 838:59:59.00000
select cast(8385959.1 as time(6));          -- 838:59:59.100000

select cast(11111111235959.999999 as time(5)); -- 24:00:00.00000
select cast(11111111235959.999999 as time(6)); -- 23:59:59.999999

select cast(-8385959.9999994 as time(5));    -- 838:59:59.00000
select cast(-8385959.9999995 as time(5));    -- -838:59:59.00000, warning
select cast(-8385959.999999 as time(6));     -- -838:59:59.999999
select cast(-8385959.9999994 as time(6));    -- -838:59:59.999999
select cast(-8385959.9999995 as time(6));    -- -838:59:59.000000, warning
select cast(-8385959.1 as time(5));          -- -838:59:59.00000
select cast(-8385959.1 as time(6));          -- -838:59:59.100000

set sql_mode = default;
create table nt6(a time(6));

insert into nt6 values (8385959.999999);        -- error
insert into nt6 values (8385959.9999994);       -- error
insert into nt6 values (8385959.9999995);       -- error
insert into nt6 values (8385959.1);             -- error
insert into nt6 values (11111111235959.999999); -- notice
insert into nt6 values (-8385959.999999);       -- error
insert into nt6 values (-8385959.9999994);      -- error
insert into nt6 values (-8385959.9999995);      -- error
insert into nt6 values (-8385959.1);            -- error

truncate nt6;
set sql_mode = '';
insert into nt6 values (8385959.999999);        -- warning
insert into nt6 values (8385959.9999994);       -- warning
insert into nt6 values (8385959.9999995);       -- warning
insert into nt6 values (8385959.1);             -- warning
insert into nt6 values (11111111235959.999999); -- notice
insert into nt6 values (-8385959.999999);       -- warning
insert into nt6 values (-8385959.9999994);      -- warning
insert into nt6 values (-8385959.9999995);      -- warning
insert into nt6 values (-8385959.1);            -- warning

select * from nt6 order by a;

set sql_mode = default;
drop table nt6;