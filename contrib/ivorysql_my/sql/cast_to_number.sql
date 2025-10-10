create table src (s text);
insert into src values ('');
insert into src values ('a');
insert into src values ('-a');
insert into src values ('-0');
insert into src values ('-0abc');
insert into src values ('-00009');
insert into src values ('-1');
insert into src values ('-10');
insert into src values ('99999999999999999999999');
insert into src values ('99999999999999999999999abc');
insert into src values ('-9999999');
insert into src values ('-9999999abc');
insert into src values ('-99999999999999999999999');
insert into src values ('-99999999999999999999999abc');

-- int1
insert into src values ('-128');
insert into src values ('-129');
insert into src values ('127');
insert into src values ('128');
insert into src values ('255');
insert into src values ('256');

-- int2
insert into src values ('-32768');
insert into src values ('-32769');
insert into src values ('32767');
insert into src values ('32768');
insert into src values ('65535');
insert into src values ('65536');

-- int3
insert into src values ('-8388608');
insert into src values ('-8388609');
insert into src values ('8388607');
insert into src values ('8388608');
insert into src values ('16777215');
insert into src values ('16777216');

-- int4
insert into src values ('-2147483648');
insert into src values ('-2147483649');
insert into src values ('2147483647');
insert into src values ('2147483648');
insert into src values ('4294967295');
insert into src values ('4294967296');

-- int8
insert into src values ('-9223372036854775808');
insert into src values ('-9223372036854775809');
insert into src values ('9223372036854775807');
insert into src values ('9223372036854775808');
insert into src values ('18446744073709551615');
insert into src values ('18446744073709551616');

-- special
insert into src values ('-9223372036854775809');
insert into src values ('-18446744073709551614');
insert into src values ('-18446744073709551615');
insert into src values ('-18446744073709551616');
insert into src values ('-18446744073709551617');
insert into src values ('-99999999999999999999999');
insert into src values ('-9999999999999999999999998');
insert into src values ('-9999999999999999999999999');

--
-- text to uint8
--
select s, cast(s as unsigned) from src; -- 35 warnings

create table dst(s text, v int8 unsigned);
set sql_mode = '';
insert into dst select s, s from src;      -- 32 warnings
select * from dst;
set sql_mode = default;
drop table dst;

--
-- text to uint4
--
--select s, cast(s as uint4) from src;

create table dst(s text, v int4 unsigned);
set sql_mode = '';
insert into dst select s, s from src;      -- 36 warnings
select * from dst;
set sql_mode = default;
drop table dst;

--
-- text to uint2
--
--select s, cast(s as uint2) from src;

create table dst(s text, v int2 unsigned);
set sql_mode = '';
insert into dst select s, s from src;      -- 44 warnings
select * from dst;
set sql_mode = default;
drop table dst;

--
-- text to uint1
--
--select s, cast(s as uint1) from src;

create table dst(s text, v tinyint unsigned);
set sql_mode = '';
insert into dst select s, s from src;      -- 48 warnings
select * from dst;
set sql_mode = default;
drop table dst;

--
-- text to int8
--
select s, cast(s as signed) from src;      -- 21 warnings

create table dst(s text, v int8);
set sql_mode = '';
insert into dst select s, s from src;      -- 21 warnings
select * from dst;
set sql_mode = default;
drop table dst;

--
-- text to int4
--
--select s, cast(s as signed) from src;

create table dst(s text, v int);
set sql_mode = '';
insert into dst select s, s from src;      -- 27 warnings
select * from dst;
set sql_mode = default;
drop table dst;

--
-- text to int2
--
--select s, cast(s as signed) from src;

create table dst(s text, v int2);
set sql_mode = '';
insert into dst select s, s from src;      -- 40 warnings
select * from dst;
set sql_mode = default;
drop table dst;



drop table src;


select cast('-0'::text as signed);      -- no warning
select cast('-0a'::text as signed);     -- warning: truncated
select cast('-a'::text as signed);      -- warning: truncated

select cast('-0'::text as unsigned);    -- warning: cast
select cast('-0a'::text as unsigned);   -- 2 warings: truncate & cast
select cast('-a'::text as unsigned);    -- warning: truncated