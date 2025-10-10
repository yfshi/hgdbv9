create table hex_test as select 0x11;
\d hex_test
drop table hex_test;

set mysql.cast_to_char = on;

select 0x;
select 0X;
select 0x1;
select 0X1; -- parse as column
select 0xg; -- parse as column
select 0Xg; -- parse as column

select cast(0x3233 as char);                    -- 23
select cast(0x3233 as nchar);                   -- 23
select cast(0x11 as signed);                    -- 17
select cast(0x11 as unsigned);                  -- 17
select cast(0x11 as decimal);                   -- 17
select cast(0x11 as binary);                    -- \x11
select cast(0x3230323231323132 as date);        -- 2022-12-12
select cast(0x20203230323231323132 as date);    -- 2022-12-12
select cast(0x3230323231323132 as datetime);    -- 2022-12-12 00:00:00
select cast(0x313231323132 as time);            -- 12:12:12

select cast(0x8000000000000000 as signed);      -- -9223372036854775808
select cast(0x000000000000000000000000000000000000000001 as signed);        -- 1
select cast(0x100000000000000000000000000000000000000001 as signed);        -- -1 and warning
select cast(0x100000000000000000000000000000000000000001 as unsigned);
select cast(0x100000000000000000000000000000000000000001 as decimal(64));
select cast(0x00 as signed);    -- 0
select cast(0x00 as unsigned);  -- 0
select cast(0x11111111111111111 as signed);     -- -1
select cast(0x11111111111111111 as unsigned);   -- 18446744073709551615

create table hex_test(ii int1, i2 int2, i3 int3, i4 int4, i8 int8);
insert into hex_test values (0x3, 0x3, 0x3, 0x3, 0x3);
select * from hex_test;
drop table hex_test;

create table hex_test(u1 uint1, u2 uint2, u3 uint3, u4 uint4, u8 uint8);
insert into hex_test values (0x3, 0x3, 0x3, 0x3, 0x3);
select * from hex_test;
drop table hex_test;

create table hex_test(a int, b text, c char(10), d varchar(10), e varbinary(111), f float4, g float8);
insert into hex_test values (0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3);
insert into hex_test values (0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);
insert into hex_test values (0x003, 0x003, 0x003, 0x003, 0x003, 0x003, 0x003);
insert into hex_test values (0x033, 0x033, 0x033, 0x033, 0x033, 0x033, 0x033);
insert into hex_test values (0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33);
select * from hex_test;
drop table hex_test;

/* Begin - bug#z5411 */
create table t1(id int3 unsigned);
insert into t1 values(0x123);
select * from t1;

create table t2(a int2 unsigned);
insert into t2 values(0x123);
select * from t2;

create table t3(a int4 unsigned);
insert into t3 values(0x123);
select * from t3;

create table t4(a int8 unsigned);
insert into t4 values(0x123);
select * from t4;

drop table t1, t2, t3, t4;
/* End - bug#z5411 */

select 0x7fffffffffffffff + 1;  -- 9223372036854775808
select 0xfffffffffffffffe + 1;  -- 18446744073709551615
select 0xffffffffffffffff + 1;  -- should be out-of-range error

set sql_mode = '';
select 0x323232323232323232323232323232 || '1';    -- waring, and result is 1

set sql_mode = 'pipes_as_concat';
select 0x323232323232323232323232323232 || 'a';    -- no warning, and result is 1
reset sql_mode;

select 0x11111111111111111;     -- no warning
select !0x11111111111111111;    -- 1 warning
select 0x11111111111111111 and 0x22222222222222222;     -- 2 warnings for bool express
select 0x11111111111111111 and !0x22222222222222222;    -- 2 warnings
select 0x22222222222222222 and !0x11111111111111111;    -- 2 warnings
select !0x11111111111111111 and 0x22222222222222222;    -- mysql only 1 warning
select !0x11111111111111111 and !0x22222222222222222;   -- mysql only 1 warning

select 0x222222222222222224  = 0x222222222222222224;    -- no warning, return true
select 0x222222222222222224  = 0x0222222222222222224;   -- no warning, return false for length mismatch
select 0x00222222222222222224  = 0x0222222222222222224; -- no warning, return true

select 0x222222222222222223  = 0x222222222222222224;    -- no warning, return false
select 0x222222222222222223 != 0x222222222222222224;    -- no warning, return true

-- test function parameter with hex
select to_days(0x313131313131313131313131);         -- as binary, no warning
select to_seconds(0x313131313131313131313131);      -- as binary, no warning
select dayname(0x313131313131313131313131);         -- as binary, no warning
select monthname(0x313131313131313131313131);       -- as binary, no warning
select time_to_sec(0x313131313131313131313131);     -- as binary, no warning
select date(0x313131313131313131313131);            -- as binary, no warning
select week(0x313131313131313131313131);            -- as binary, no warning
select yearweek(0x313131313131313131313131);        -- as binary, no warning
select from_days(0x313131313131313131313131);       -- as number, warning
SELECT lcase(0x313131313131313131313131);           -- as binary, no warning
SELECT ucase(0x313131313131313131313131);           -- as binary, no warning
select bin(0x313131313131313131313131);             -- as binary, no warning
select char(0x313131313131313131313131);            -- as number, warning
select hex(0x313131313131313131313131);             -- as binary, no warning
select SPACE(0x31);                                 -- as number
SELECT ORD(0x313131313131313131313131);             -- as binary, no warning
SELECT SOUNDEX(0x313131313131313131313131);         -- as binary, no warning
SELECT LENGTH(0x313131313131313131313131);          -- as binary, no warning
SELECT BIT_LENGTH(0x313131313131313131313131);      -- as binary, no warning
SELECT OCTET_LENGTH(0x313131313131313131313131);    -- as binary, no warning
select get_format(DATE, 0x455552);                  -- as binary, no warning
select elt(0x313131313131313131313131, 'Aa', 'Bb', 'Cc', 'Dd'); -- as number
select elt(1, 0x4161, 0x4262, 0x4363, 0x4164);      -- as binary, no warning
SELECT find_in_set(0x62, 'a,b,c');                  -- as binary, no warning
SELECT find_in_set(0x62, 0x612C622C63);             -- as binary, no warning
SELECT from_base64(0x614756736247383D);             -- as binary, no warning
SELECT to_base64(0x68656C6C6F);                     -- as binary, no warning
SELECT left('abcdef', 0x03);                        -- as number
SELECT left(0x616263646566, 3);                     -- as binary, no warning
SELECT left(0x616263646566, 0x03);
SELECT right('abcdef', 0x03);                       -- as number
SELECT right(0x616263646566, 3);                    -- as binary, no warning
SELECT right(0x616263646566, 0x03);
select datediff(0x323030372D31322D33312032333A35393A3539, 0x323030372D31322D3330); -- as binary, no warning
select timestampadd(month, 0x01, 0x323031392D312D3330); -- as binary, no warning

-- operator
select
    case 0xffffffffffffffffff
        when '1'
            then true 
        when 2
            then false 
        else null 
    end as x;

select
    case 0xffffffffffffffffff
        when '1'
            then true 
        when '2'
            then false 
        else null 
    end as x;

-- other
select cast(-1 as signed);
select cast(-1 as unsigned);

select x'1';
select X'1';
select x'01';
select X'01';

reset mysql.cast_to_char;