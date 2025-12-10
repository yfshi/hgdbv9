
SET mysql.use_mysql_bit_literal TO on;

create table t (c bit);
\d t;
drop table t;
create table t (c bit(0));
create table t (c bit(65));
create table t (c bit(32));
\d t;

insert into t values(1234);
insert into t values('abcd');
insert into t values(true);
insert into t values('11');
insert into t values(0x12345678);
insert into t values(0x12005678);

COPY t FROM stdin;
0x12
0x1234
0x12340078
0x12345678
\.

select * from t;
select c > c from t;
\gdesc
select c >= c from t;
\gdesc
select c < c from t;
\gdesc
select c <> c from t;
\gdesc
select c <= c from t;
\gdesc
select c <=> c from t;
\gdesc
select c = c from t;
\gdesc
select c like c from t;
\gdesc
select c not like c from t;
\gdesc
select c & c from t;
\gdesc
select c >> c from t;
\gdesc
select c << c from t;
\gdesc
select c ^ c from t;
\gdesc
select c | c from t;
\gdesc
select ~c from t;
\gdesc
select bit_count(c) from t;
select BIT_COUNT(c) from t;

select +c from t;
\gdesc
select -c from t;
\gdesc

select c % c from t;
\gdesc
select c mod c from t;
\gdesc
select c + c from t;
\gdesc
select c - c from t;
\gdesc
select c / c from t;
\gdesc
select c div c from t;
\gdesc
select c * c from t;
\gdesc

select c and c from t;
\gdesc
select c && c from t;
\gdesc
select !c from t;
\gdesc
select not c from t;
\gdesc
select c or c from t;
\gdesc
set sql_mode = 'ANSI_QUOTES';
select c || c from t;
\gdesc
reset sql_mode;
select c || c from t;
\gdesc
select c xor c from t;
\gdesc

select binary(c) from t;
select c + 0 from t;
select c is true from t;
select c is false from t;
select c is not true from t;
select c is not false from t;
drop table t;

create table t (c bit(5));
insert into t values(b'1010');
insert into t values('a');
insert into t values('');
select * from t;
drop table t;

create table t (c bit(10));
insert into t values(b'1010');
insert into t values('a');
insert into t values('');
select * from t;
drop table t;

create table t (c bit(18));
insert into t values(b'1010');
insert into t values('ab');
insert into t values('');
select * from t;
drop table t;

CREATE TABLE BIT_TABLE(b BIT(11));
INSERT INTO BIT_TABLE VALUES (B'10');
INSERT INTO BIT_TABLE VALUES (B'00000000000');
INSERT INTO BIT_TABLE VALUES (B'11011000000');
INSERT INTO BIT_TABLE VALUES (B'01010101010');
INSERT INTO BIT_TABLE VALUES (B'101011111010'); -- too long
SELECT * FROM BIT_TABLE;
drop table BIT_TABLE;

create table t_bit(a bit(8));
set sql_mode = '';
insert into t_bit values (0xffff);  -- warning
set sql_mode = default;
insert into t_bit values (0xffff);  -- error
select hex(a) from t_bit;           -- 0xFF
drop table t_bit;

create table t_bit(a bit(13));
set sql_mode = '';
insert into t_bit values (0xffff);  -- warning
set sql_mode = default;
insert into t_bit values (0xffff);  -- error
select hex(a) from t_bit;           -- 0x1FFF
drop table t_bit;


create table t_bit(a bit(8));
set sql_mode = '';
insert into t_bit values (0xffff);  -- warning
set sql_mode = default;
insert into t_bit values (0xffff);  -- error
select hex(a) from t_bit;           -- 0xFF
drop table t_bit;

create table t_bit(a bit(13));
set sql_mode = '';
insert into t_bit values (0xffff);  -- warning
set sql_mode = default;
insert into t_bit values (0xffff);  -- error
select hex(a) from t_bit;           -- 0x1FFF
drop table t_bit;

-- bug 5691
create table t_bit_function (b1 BIT(8), b2 bit(17), b3 BIT(32), b4 BIT(63),b5 BIT(1));
insert into t_bit_function values ('A', 1234, b'101010',0x1234,true);
insert into t_bit_function values (b'0', b'0', b'0', b'0', b'0');
select binary(b1), binary(b2), binary(b3), binary(b4), binary(b5) from t_bit_function;
\gdesc
drop table t_bit_function;

-- bug 5669 5670
create table t_bit_operator (b3 BIT(32));
insert into t_bit_operator values (b'101010');
select b3 = b'101010' from t_bit_operator;
select b3 != b'101010' from t_bit_operator;
select b3 <> b'101010' from t_bit_operator;
drop table t_bit_operator;

-- bug 5671
create table t_bit_operator (b3 BIT(32),b4 BIT(63),b5 BIT(1));
insert into t_bit_operator values (b'101010',0x1234,true);
select b3 >= b'101010' from t_bit_operator;
select b5>=b4 from t_bit_operator;
drop table t_bit_operator;

-- bug 5675 5676 5677
create table t_bit_operator (b1 BIT(8));
insert into t_bit_operator values ('A');
select b1 & b'11110000' from t_bit_operator limit 0,1;
SELECT b1 ^ b'11111111' FROM t_bit_operator;
SELECT b1 | b'11111111' FROM t_bit_operator;
drop table t_bit_operator;

-- bug 5673 5674
create table t_bit_operator (b4 BIT(63), b5 BIT(1));
insert into t_bit_operator values (0x1234,true);
select b5 < b4 from t_bit_operator;
select b5 <= b4 from t_bit_operator;
drop table t_bit_operator;

-- bug 5704
create table t_bit_operator (b3 BIT(64), b4 BIT(63), b5 BIT(1));
insert into t_bit_operator values (X'FFFF',0x1234,true);
SELECT b5>b4, b5>b3 from t_bit_operator;
drop table t_bit_operator;

-- bug 5678
create table t_bit_operator (b1 BIT(8), b2 bit(17));
insert into t_bit_operator values ('A', 123);
select b1 % b2, MOD (b1, b2) from t_bit_operator;
drop table t_bit_operator;

-- bug 5688
create table t_index_brin (id serial primary key, flag bit(64));
insert into t_index_brin (flag) select (i % 65536) from generate_series(1, 100000) as i;
set enable_seqscan = off;
create index idx_brin on t_index_brin using brin (flag);
explain (costs off) select * from t_index_brin where flag=b'1010';
drop index idx_brin;
drop table t_index_brin;

-- bug 5686
create table t_index_btree (id serial primary key, flag bit(64));
insert into t_index_btree (flag) select (i % 65536) from generate_series(1, 100000) as i;
create index  idx_btree on t_index_btree  (flag);
explain (costs off) select * from t_index_btree where flag=b'1010';
drop index idx_btree;
drop table t_index_btree;

-- bug 5661
create table t_grammer_6(id BIT(11.8));
\d t;
create table t_grammer_7(id BIT(64.5));
\d t;
drop table t_grammer_6, t_grammer_7;

-- bug 5697
create table t (c bit(3));
insert into t values (0x1);
select * from t;
drop table t;
create table t (c bit(5));
insert into t values (0x1);
select * from t;
drop table t;

-- bug 5694 5696 5668
CREATE TABLE public.t_bit_dump_restore (b1 mysbit(8), b2 mysbit(51), b3 mysbit(32), 
										b4 mysbit(63), b5 mysbit(1));
INSERT INTO public.t_bit_dump_restore VALUES (0xAA, 0x00E7809AE9AB98, 0x0FFFFFFF, 0x0000402321402321, 0x00);
select * from public.t_bit_dump_restore;
drop talbe public.t_bit_dump_restore;

-- bug 5698 
create table t_convert_uint8_bit(id int8 unsigned);
insert into t_convert_uint8_bit values(0),(100),(18446744073709551615);
set sql_mode = '';
select cast(id as bit(1)) from t_convert_uint8_bit;
select cast(id as bit(11)) from t_convert_uint8_bit;
select cast(id as bit(63)) from t_convert_uint8_bit;
set sql_mode = default;
select cast(id as bit(1)) from t_convert_uint8_bit;
select cast(id as bit(11)) from t_convert_uint8_bit;
select cast(id as bit(63)) from t_convert_uint8_bit;
drop table t_convert_uint8_bit;

-- bug 5664
create table t_bit(b1 BIT(8), b2 bit(17), b3 BIT(32), b4 bit(63), b5 bit(64));
insert into t_bit(b1) values(-4);
insert into t_bit(b2) values(-4);
insert into t_bit(b3) values(-4);
insert into t_bit(b4) values(-4);
insert into t_bit(b5) values(-47868678990);
insert into t_bit(b1) values(120120);
select * from t_bit;
set sql_mode = '';
delete from t_bit;
insert into t_bit(b1) values(-4);
insert into t_bit(b2) values(-4);
insert into t_bit(b3) values(-4);
insert into t_bit(b4) values(-4);
insert into t_bit(b5) values(-47868678990);
insert into t_bit(b1) values(120120);
select * from t_bit;
set sql_mode = default;
drop table t_bit;

-- bug 5693 
set sql_mode = '';
create table t_bit_sql_mode (b1 BIT(8), b2 bit(17), b3 BIT(32), b4 BIT(63), b5 BIT(1));
insert into t_bit_sql_mode values (0b101010101010,'这是一条测试数据',X'FFFFFFFFFFFFFFFFFFFFFFFFFFFFFF','@#!@#!%^&*特殊字符',10);
select * from t_bit_sql_mode;
set sql_mode = default;
drop table t_bit_sql_mode;

-- bug 5667
set sql_mode = '';
create table t_bit(b1 BIT(8), b2 bit(15), b3 BIT(32), b4 bit(63));
insert into t_bit(b1) value(0xFFFFFFFFFFFFFFFFFFFFFFF);
set sql_mode = default;
insert into t_bit(b1) value(0xFFFFFFFFFFFFFFFFFFFFFFF);
select * from t_bit;
drop table t_bit;

-- bug 5689
create table t_index_hash(id serial primary key, flag bit(64));
insert into t_index_hash (flag) select (i % 65536) from generate_series(1, 100000) as i;
set enable_seqscan = off;
create index idx_hash on t_index_hash using hash(flag);
explain (costs off) select * from t_index_hash where flag = '0x12';
drop index idx_hash;
drop table t_index_hash;

-- bug 5805
select convert('2023-08-20', unsigned),convert('2025-03-24 20:47:35', unsigned),convert(b'1101', unsigned);

SET mysql.use_mysql_bit_literal TO off;

-- bug 6613
create table t_bit_operator(b1 BIT(8), b2 bit(17), b3 BIT(32), b4 BIT(63), b5 BIT(1)); 
insert into t_bit_operator values ('A', 1234, b'101010',0x1234,true);
select mod(b4,3) from t_bit_operator; 
drop table t_bit_operator;