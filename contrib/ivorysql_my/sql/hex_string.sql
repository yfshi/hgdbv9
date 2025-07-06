create table hex_test as select 0x11;
\d hex_test
drop table hex_test;

set mysql.cast_to_char = on;

select cast(0x3233 as char);                      -- 23
select cast(0x3233 as nchar);                     -- 23
select cast(0x11 as signed);                    -- 17
select cast(0x11 as unsigned);                  -- 17
select cast(0x11 as decimal);                   -- 17
select cast(0x11 as binary);                    -- \x11
select cast(0x3230323231323132 as date);        -- 2022-12-12
select cast(0x20203230323231323132 as date);        -- 2022-12-12
select cast(0x3230323231323132 as datetime);    -- 2022-12-12 00:00:00
select cast(0x313231323132 as time);            -- 12:12:12

select cast(0x8000000000000000 as signed);      -- -9223372036854775808
select cast(0x000000000000000000000000000000000000000001 as signed);        -- 1
select cast(0x100000000000000000000000000000000000000001 as signed);        -- -1 and warning
select cast(0x100000000000000000000000000000000000000001 as unsigned);
select cast(0x100000000000000000000000000000000000000001 as decimal);
select cast(0x00 as signed);    -- 0
select cast(0x00 as unsigned);  -- 0
select cast(0x11111111111111111 as signed);     -- -1
select cast(0x11111111111111111 as unsigned);   -- 18446744073709551615

create table hex_test(a int, b text, c char(10), d varchar(10), e varbinary(111), f float4, g float8);
insert into hex_test values (0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3);
insert into hex_test values (0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);
insert into hex_test values (0x003, 0x003, 0x003, 0x003, 0x003, 0x003, 0x003);
insert into hex_test values (0x033, 0x033, 0x033, 0x033, 0x033, 0x033, 0x033);
insert into hex_test values (0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33);
select * from hex_test;
drop table hex_test;


select 0x7fffffffffffffff + 1;  -- 9223372036854775808
select 0xfffffffffffffffe + 1;  -- 18446744073709551615
select 0xffffffffffffffff + 1;  -- should be out-of-range error

select 0x11111111111111111;     -- no warning
select !0x11111111111111111;    -- 1 warning
select 0x11111111111111111 and 0x22222222222222222;     -- 2 warnings for bool express
select 0x11111111111111111 and !0x22222222222222222;    -- 2 warnings
select 0x22222222222222222 and !0x11111111111111111;    -- 2 warnings
select !0x11111111111111111 and 0x22222222222222222;    -- mysql only 1 warning, but we have 2
select !0x11111111111111111 and !0x22222222222222222;   -- mysql only 1 warning, but we have 2

reset mysql.cast_to_char;
