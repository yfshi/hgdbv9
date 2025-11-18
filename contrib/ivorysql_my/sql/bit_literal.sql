
set mysql.use_mysql_bit_literal to on;

set mysql.cast_to_char = on;


select 0b;
select 0B;
select 0b1;
select 0B1;
select 0ba;
select 0Bc;
select 0b01100001;
select B'111';
select B'0010';
select B'123';
select B'100A100';
select b'01100100';


select cast(0b01100001 as char);                -- a
select cast(0b01100110 as nchar);               -- f
select cast(0b01 as signed);                    -- 1
select cast(0b10 as unsigned);                  -- 2
select cast(0b11111111 as decimal);             -- 255
select cast(0b01111010 as binary);              -- \x7a
select cast(0b0011001000110000001100100011001000110001001100100011000100110010 as date);        -- 2022-12-12
select cast(0b0011001000110000001100100011001000110001001100100011000100110010 as datetime);    -- 2022-12-12 00:00:00
select cast(0b001100010011001000110001001100100011000100110010 as time);                        -- 12:12:12 

select cast(0b1111111111111111 as signed);                                                        -- 65535
select cast(0b1111111111111111111111111111111111111111111111111111111111111111 as signed);        -- 1
select cast(0b11111111111111111111111111111111111111111111111111111111111111111 as signed);       -- -1 and warning
select cast(0b1111111111111111111111111111111111111111111111111111111111111111 as unsigned);      -- 18446744073709551615
select cast(0b11111111111111111111111111111111111111111111111111111111111111111 as decimal(64));  -- 18446744073709551615 and warning
select cast(0b00 as signed);    -- 0
select cast(0b0000 as unsigned);  -- 0



create table bit_test(i1 int1, i2 int2, i3 int3, i4 int4, i8 int8);
insert into bit_test values (0b11, 0b11, 0b11, 0b11, 0b11);
select * from bit_test;
drop table bit_test;

create table bit_test(u1 uint1, u2 uint2, u3 uint3, u4 uint4, u8 uint8);
insert into bit_test values (0b11, 0b11, 0b11, 0b11, 0b11);
select * from bit_test;
drop table bit_test;


create table t1(a int3 unsigned);
insert into t1 values(0b01100001);
select * from t1;

create table t2(a int2 unsigned);
insert into t2 values(0b01100001);
select * from t2;

create table t3(a int4 unsigned);
insert into t3 values(0b01100001);
select * from t3;

create table t4(a int8 unsigned);
insert into t4 values(0b01100001);
select * from t4;

drop table t1, t2, t3, t4;


select 0b1111111111111111111111111111111111111111111111111111111111111111 + 1;  -- 0
select 0b11111111111111111111111111111111111111111111111111111111111111111 + 0; -- 18446744073709551615 + warning


set sql_mode = '';
select 0b11111111 || '1';   -- t
select 0b11111111 || '0';  -- t
select 0b11111111 ||  1 ;  -- t
select 0b11111111 ||  0 ;   -- t
select 0b11111111 ||  true;    -- t
select 0b11111111 ||  false;    -- t


set sql_mode = 'pipes_as_concat';
select 0b01100110 || 'a';   -- fa
reset sql_mode;


create table bit_test as select 0b11;
\d bit_test
drop table bit_test;


select position(B'1010' IN B'00001010');           -- 1
select position(B'111010110' IN B'0111010110');    -- 1
select position(B'1010' in B'11111010');           -- 0


reset mysql.cast_to_char;

SET mysql.use_mysql_bit_literal TO off;