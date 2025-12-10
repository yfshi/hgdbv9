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

select cast('!@#' as decimal(6, 3));    -- 1 warning
select cast('123!@#' as decimal(6, 3)); -- no warning


create table test_convert_signed (
    original_value varchar(50),
    converted_value bigint signed
);

set sql_mode='';

insert into test_convert_signed (original_value, converted_value) values 
('9223372036854775808', 9223372036854775808), 
('-9223372036854775809', -9223372036854775809);

-- insert into test_convert_signed (original_value, converted_value) values
-- ('9223372036854775808', convert(9223372036854775808, signed)),
-- ('-9223372036854775809', convert(-9223372036854775809, signed));

select * from test_convert_signed;
drop table test_convert_signed;


select convert(9223372036854775807, signed);        -- 9223372036854775807
select convert(9223372036854775808, signed);        -- -9223372036854775808
select convert(9223372036854775808.0, signed);      -- 9223372036854775807, warning
select convert(9223372036854775808.9, signed);      -- 9223372036854775807, warning
select convert(9223372036854775809, signed);        -- -9223372036854775807
select convert(9223372036854775809.0, signed);      -- 9223372036854775807, warning
select convert(-9223372036854775809, signed);       -- -9223372036854775808, warning

select convert(-1, unsigned);                           -- 18446744073709551615
select convert(9223372036854775808.9, unsigned);        -- 9223372036854775809
select convert(-9223372036854775808, unsigned);         -- 9223372036854775808
select convert(-9223372036854775808.0, unsigned);       -- 0, warning
select convert(-9223372036854775808.9, unsigned);       -- 0, warning
select convert(-9223372036854775809, unsigned);         -- 0, warning

create table test_convert_decimal (
    original_value varchar(50),
    converted_value decimal(15,2)
);

insert into test_convert_decimal (original_value, converted_value) values
('123456789.12', convert('123456789.12', decimal(10,2)));

select * from test_convert_decimal;

reset sql_mode;

/* Begin - bug#6533 */
select convert(true, float),  convert(false, float),  convert('true', float),  convert('false', float);
select convert(1 = 1, float), convert(1 = 2, float), convert(NULL IS NULL, float),  convert('a' = 'b', float);
/* End - bug#6533 */

/* Begin - bug#6531 */
select convert(true, double) ,  convert(false, double),  convert('true', double), convert('false', double);
select convert(1 = 1, double),  convert(1 = 2, double),   convert(NULL IS NULL, double),  convert('a' = 'b', double);
/* End - bug#6531 */

/* Begin - bug#6529 */
select convert(true, signed),convert(false, signed),convert(TRUE, signed),  convert(FALSE, signed);
select convert(1 = 1, signed), convert(1 = 2, signed),  convert(NULL IS NULL, signed),   convert('a' = 'b', signed);
/* End - bug#6529 */

/* Begin - bug#6528 */
select convert(TRUE, decimal(10,2)) ,  convert(FALSE, decimal(10,2));
select convert(1 = 1, decimal(10,2)),  convert(1 = 2, decimal(10,2)), convert(NULL IS NULL, decimal(10,2)),  convert('a' = 'b', decimal(10,2));
/* End - bug#6528 */