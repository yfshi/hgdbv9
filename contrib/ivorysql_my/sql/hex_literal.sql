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
select cast(0x32333435 as char(3));             -- 234, warning
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

/* Begin - bug#z6026 */
create table t_hex_convert_string (c_char char(2));
insert into t_hex_convert_string values ( x'414243');
set sql_mode ='';
insert into t_hex_convert_string values ( x'414243');
select * from t_hex_convert_string;
drop table t_hex_convert_string;

create table t_hex_convert_string (c_varchar varchar(5));
insert into t_hex_convert_string values (X'414243444546');
select * from t_hex_convert_string;
drop table t_hex_convert_string;

reset sql_mode;
/* End - bug#z6026 */

/* Begin - bug#z6027 */
create table t_hex_to_enum (c_enum enum(x'123'));
create table t_hex_to_enum (c_enum enum(x'1234'));
drop table t_hex_to_enum;
create table t_hex_to_enum (c_enum enum(0x323334));
drop table t_hex_to_enum;
create table t_hex_to_enum (c_enum enum(0x3233340035));
drop table t_hex_to_enum;
/* End - bug#z6027 */

/* Begin - bug#z6028 */
create table  t_hex_to_time (
    timestamp_col timestamp,
	year_col year
);

insert into t_hex_to_time values(0x3230323331303031313233303435, X'03');
select * from t_hex_to_time;
drop table t_hex_to_time;
/* End - bug#z6028 */

/* Begin - bug#z6031 */
select +0x123, +X'ffac', +x'1234';
/* End - bug#z6031 */

/* Begin - bug#z6033 */
select 3::uint8 % 2::uint8, pg_typeof(3::uint8 % 2::uint8);         -- 1, uint8

select 3::uint8 % 2::int2, pg_typeof(3::uint8 % 2::int2);           -- 1, uint8
select 3::uint8 % (-2)::int2, pg_typeof(3::uint8 % (-2)::int2);     -- 1, uint8

select 3::uint8 % 2::int4, pg_typeof(3::uint8 % 2::int4);           -- 1, uint8
select 3::uint8 % (-2)::int4, pg_typeof(3::uint8 % (-2)::int4);     -- 1, uint8

select 3::uint8 % 2::int8, pg_typeof(3::uint8 % 2::int8);           -- 1, uint8
select 3::uint8 % (-2)::int8, pg_typeof(3::uint8 % (-2)::int8);     -- 1, uint8

select 3::int2 % 2::uint8, pg_typeof(3::int2 % 2::uint8);           -- 1, int8
select (-3)::int2 % 2::uint8, pg_typeof((-3)::int2 % 2::uint8);     -- -1, int8

select 3::int4 % 2::uint8, pg_typeof(3::int4 % 2::uint8);           -- 1, int8
select (-3)::int4 % 2::uint8, pg_typeof((-3)::int4 % 2::uint8);     -- -1, int8

select 3::int8 % 2::uint8, pg_typeof(3::int8 % 2::uint8);           -- 1, int8
select (-3)::int8 % 2::uint8, pg_typeof((-3)::int8 % 2::uint8);     -- -1, int8
/* End - bug#z6033 */

/* Begin - bug#z6039 */
create table t_numeric_arith_hex ( unsigned_tinyint tinyint unsigned ); 
insert into t_numeric_arith_hex values (12);
select unsigned_tinyint * (-x'12ffac') from t_numeric_arith_hex;
drop table t_numeric_arith_hex;
/* End - bug#z6039 */

/* Begin - bug#z6037 */
select '-32000'::smallint + 0x1234;
select 1 + 0xffffffffffffffff;
select -1000 + 0xff;
/* End - bug#z6037 */

/* Begin - bug#z6041 */
create table t_numeric_arith_hex (float_col float, double_col double, decimal_col decimal(20,6)); 
insert into t_numeric_arith_hex values (3.25, -2.75, 12345.678901);

select
    0x1234ff && float_col,
    0x1234ff && double_col,
    0x1234ff and decimal_col
from t_numeric_arith_hex;

drop table t_numeric_arith_hex;
/* End - bug#z6041 */

/* Begin - bug#z6052 */
create table idx_hex ( b_fixed binary(20), b_var varbinary(20) ); 
insert into idx_hex values(1,2);
select * from idx_hex where b_var=0x32;
drop table idx_hex;
/* End - bug#z6052 */

/* Begin - bug#z6045 */
create table t_hex_to_time (date_col date);
insert into t_hex_to_time values ( x'323032332d31302d3031' );

select
    date_col > 0x323032332d31302d3031,
    date_col >= 0x323032332d31302d3031,
    date_col < 0x323032332d31302d3031,
    date_col <> 0x323032332d31302d3031,
    date_col != 0x323032332d31302d3031,
    date_col <= 0x323032332d31302d3031,
    date_col <=> 0x323032332d31302d3031,
    date_col = 0x323032332d31302d3031,
    date_col like '2020-01%',
    date_col not like '2020-01%'
from t_hex_to_time ;

drop table t_hex_to_time;
/* End - bug#z6045 */

/* Begin - bug#z6040 */
create table t_numeric_arith_hex ( float_col float );
insert into t_numeric_arith_hex values (3.25);
select float_col + x'ffffffff' from t_numeric_arith_hex;    -- 4294967298.25
select x'1234' - float_col from t_numeric_arith_hex;        -- 4656.75
select float_col * X'ff'   from t_numeric_arith_hex;        -- 828.75
select x'FF1234' / float_col from t_numeric_arith_hex;      -- 5143489.230769231
select -X'ffffccdd' % float_col from t_numeric_arith_hex;   -- -2.5
select float_col mod 0x1234 from t_numeric_arith_hex;       -- 3.25
select float_col div 0x12F from t_numeric_arith_hex;        -- 0
select -float_col div 0x12F from t_numeric_arith_hex;       -- 0
drop table t_numeric_arith_hex;
/* End - bug#z6040 */
/* Begin - bug#z6044 */
create table t_hex_to_time ( date_col date, timestamp_col timestamp, time_col time, datetime_col datetime, year_col year );
insert into t_hex_to_time values ( x'323032332d31302d3031', 0x3230323331303031313233303435, 
x'31323a33303a3435', 0x323032352D30312D30312031323A30303A3030, X'03' );
select
    date_col + 0x10,
    date_col - 0x10,
    date_col % 0x0a,
    mod(date_col, 0x0a),
    time_col + 0x01,
    time_col - 0x01
from t_hex_to_time;
select
    time_col * 0x02,
    time_col / 0x02,
    time_col div 0x02
from t_hex_to_time;
select
    datetime_col + 0x10,
    datetime_col - 0x10,
    datetime_col % 0x0a
from t_hex_to_time;
select
    timestamp_col + 0x10,
    timestamp_col - 0x10,
    timestamp_col % 0x0a
from t_hex_to_time;
select
    year_col + 0x01,
    year_col - 0x01,
    year_col * 0x02,
    year_col / 0x02,
    year_col div 0x02
from t_hex_to_time;

drop table t_hex_to_time;
/* End - bug#z6044 */
/* Begin - bug#5378 */
CREATE TABLE tb_test (
    a FLOAT
);

INSERT INTO tb_test VALUES (3.0), (3.14), (2.9999999), (3.0000001), (NULL);

SELECT a, a <=> 3, a <=> 2023-01-01, a <=> '3.14', a <=> 0x03 FROM tb_test ORDER BY a;
SELECT a, a + 3, a + '3', a + '3.14', a + 0x03 FROM tb_test ORDER BY a;
SELECT a, a & 3, a & 0x03, a <=> '3.14', a << 1, a >> 1, a << '1', a >> '1', 
       a << 0x01, a >> 0x01, a | 1, a | '1', a ^ 1, a ^ '1' FROM tb_test ORDER BY a;
SELECT a, a * 3, a * 2023-01-01, a * '3.14', a * 0x03 FROM tb_test ORDER BY a;

drop table tb_test;
/* End - bug#5378 */
reset mysql.cast_to_char;