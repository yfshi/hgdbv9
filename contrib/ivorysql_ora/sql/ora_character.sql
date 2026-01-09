-- 'nls_length_semantics' parameter
SET nls_length_semantics to char;
SET nls_length_semantics to byte;

-- Only values of 'char' and 'byte' is valid
SET nls_length_semantics to other;

-- invalid range
CREATE TABLE TEST_ORACHAR(a char(32768 byte));

CREATE TABLE TEST_ORACHAR(a char(32768 char));

CREATE TABLE TEST_ORACHAR(a varchar2(32768 byte));

CREATE TABLE TEST_ORACHAR(a varchar2(32768 char));

-- valid range
CREATE TABLE TEST_ORACHAR(a char(5 byte), b char(5 char));

CREATE TABLE TEST_ORAVARCHAR(a varchar2(5 char), b varchar2(5 byte));

INSERT INTO TEST_ORACHAR SELECT generate_series(1,10), generate_series(1,10);

INSERT INTO TEST_ORAVARCHAR SELECT generate_series(1,10), generate_series(1,10);


-- Comparison operator
--char
SELECT * FROM TEST_ORACHAR WHERE a = '5';

SELECT * FROM TEST_ORACHAR WHERE a <> '5';

SELECT * FROM TEST_ORACHAR WHERE a > '5';

SELECT * FROM TEST_ORACHAR WHERE a >= '5';

SELECT * FROM TEST_ORACHAR WHERE a < '5';

SELECT * FROM TEST_ORACHAR WHERE a <= '5';


--varchar2
SELECT * FROM TEST_ORAVARCHAR WHERE a = '5';

SELECT * FROM TEST_ORAVARCHAR WHERE a <> '5';

SELECT * FROM TEST_ORAVARCHAR WHERE a > '5';

SELECT * FROM TEST_ORAVARCHAR WHERE a >= '5';

SELECT * FROM TEST_ORAVARCHAR WHERE a < '5';

SELECT * FROM TEST_ORAVARCHAR WHERE a <= '5';


-- Arithmetic operator
-- char/char
SELECT '123'::char(3) + '123'::char(3);

SELECT '123'::char(3) - '123'::char(3);

SELECT '123'::char(3) * '123'::char(3);

SELECT '123'::char(3) / '123'::char(3);


-- char/varchar2
SELECT '123'::char(3) + '123'::varchar2(3);

SELECT '123'::char(3) - '123'::varchar2(3);

SELECT '123'::char(3) * '123'::varchar2(3);

SELECT '123'::char(3) / '123'::varchar2(3);


-- char/unknown
SELECT '123'::char(3) + '123';

SELECT '123'::char(3) - '123';

SELECT '123'::char(3) * '123';

SELECT '123'::char(3) / '123';


-- varchar2/varchar2
SELECT '123'::varchar2(3) + '123'::varchar2(3);

SELECT '123'::varchar2(3) - '123'::varchar2(3);

SELECT '123'::varchar2(3) * '123'::varchar2(3);

SELECT '123'::varchar2(3) / '123'::varchar2(3);


-- varchar2/char
SELECT '123'::varchar2(3) + '123'::char(3);

SELECT '123'::varchar2(3) - '123'::char(3);

SELECT '123'::varchar2(3) * '123'::char(3);

SELECT '123'::varchar2(3) / '123'::char(3);


-- varchar2/unknown
SELECT '123'::varchar2(3) + '123';

SELECT '123'::varchar2(3) - '123';

SELECT '123'::varchar2(3) * '123';

SELECT '123'::varchar2(3) / '123';


-- unknown/unknown	 
SELECT '123' + '123';

SELECT '123' - '123';

SELECT '123' * '123';

SELECT '123' / '123';


-- Spaces are ignored when CHAR compared with CHAR
SELECT * FROM TEST_ORACHAR WHERE a = '5   ';

-- Spaces are not ignored when VARCHAR2 compared with VARCHAR2
SELECT * FROM TEST_ORAVARCHAR WHERE a = '5   ';

-- AGGREGATE
--CHAR
SELECT max(a), min(b) FROM TEST_ORACHAR;

SELECT min(a), max(b) FROM TEST_ORACHAR;

--VARCHAR2
SELECT max(a), min(b) from TEST_ORAVARCHAR;

SELECT min(a), max(b) from TEST_ORAVARCHAR;

-- index 
-- CHAR
DELETE FROM TEST_ORACHAR;

CREATE INDEX test_orachar_btree ON TEST_ORACHAR(a);

CREATE INDEX test_orachar_hash ON TEST_ORACHAR USING HASH (a);

CREATE INDEX test_orachar_brin ON TEST_ORACHAR USING BRIN (a);

INSERT INTO TEST_ORACHAR SELECT generate_series(1,10000), generate_series(1,10000);

SELECT * FROM TEST_ORACHAR WHERE a='111';
VACUUM ANALYZE TEST_ORACHAR;
explain (costs off) SELECT * FROM TEST_ORACHAR WHERE a='111';

--VARCHAR2
--DELETE FROM TEST_ORAVARCHAR;
TRUNCATE TEST_ORAVARCHAR;

CREATE INDEX test_oravarchar_a ON TEST_ORAVARCHAR (a);

CREATE INDEX test_oravarchar_b ON TEST_ORAVARCHAR USING HASH (a);

INSERT INTO TEST_ORAVARCHAR SELECT generate_series(1,10000), generate_series(1,10000);

SELECT * FROM TEST_ORAVARCHAR WHERE a='111';

VACUUM ANALYZE TEST_ORAVARCHAR;
--SET enable_bitmapscan = off;

explain (costs off) SELECT * FROM TEST_ORAVARCHAR WHERE a='111';

-- drop table
DROP TABLE TEST_ORACHAR;
DROP TABLE TEST_ORAVARCHAR;

/* Begin - BUG:M0000068 */
-- nchar -> oracharchar
set nls_length_semantics = 'byte';
create table test_nchar1(a nchar(3));
set nls_length_semantics = 'char';
create table test_nchar2(a nchar(3));
-- all 9000
select attrelid::regclass, attname, atttypid from pg_attribute where (attrelid = 'test_nchar1'::regclass or attrelid = 'test_nchar2'::regclass) and attname = 'a';

-- nvarchar2 -> oravarcharchar
set nls_length_semantics = 'byte';
create table test_nvarchar2_1(a nvarchar2(3));
set nls_length_semantics = 'char';
create table test_nvarchar2_2(a nvarchar2(3));
-- all 9002
select attrelid::regclass, attname, atttypid from pg_attribute where (attrelid = 'test_nvarchar2_1'::regclass or attrelid = 'test_nvarchar2_2'::regclass) and attname = 'a';


-- when used in CREATE TABLE/ALTER TABLE, but it is ok for other stmt,
-- for example used in CREATE FUNCTION stmt.
--oracle raise error,but we success
create table test_varchar2(a varchar2);
--oracle raise error, but we success
create table test_nvarchar2(a nvarchar2);
create table test_varchar2_alter(a int);
--oracle raise error,but we success
alter table test_varchar2_alter alter column a type varchar2;
--oracle raise error, but we success
alter table test_varchar2_alter alter column a type nvarchar2;
alter table test_varchar2_alter alter column a type varchar2(10);	-- ok
\d test_varchar2_alter

alter table test_varchar2_alter alter column a type nvarchar2(20);	-- ok
\d test_varchar2_alter

--oracle raise error,but we success
alter table test_varchar2_alter add column b varchar2;
--oracle raise error, but we success
alter table test_varchar2_alter add column c nvarchar2;
\d test_varchar2_alter

-- ok
create or replace function test_varchar2(a varchar2, b nvarchar2) return varchar2
is
begin
raise notice '%', a||b;
return a;
end;
/

select test_varchar2('varchar2 without typmod used in function ', 'is ok!');

-- nchar and nvarchar2 can't specify the unit of typmod
create table test_nchar3(a nchar(4 char));
create table test_nchar4(a nchar(4 byte));
create table test_nvarchar2_3(a nvarchar2(4 char));
create table test_nvarchar2_4(a nvarchar2(4 byte));

-- clean
reset nls_length_semantics;
drop table test_nchar1;
drop table test_nchar2;
drop table test_nvarchar2_1;
drop table test_nvarchar2_2;
drop function test_varchar2;
/* End - BUG:M0000068 */

/* Begin - BUG:M0000062 */
create table notoast_char1(a char(1 char));
select reltoastrelid from pg_class where relname='notoast_char1';	-- null
create table notoast_char2(a char(1 byte));
select reltoastrelid from pg_class where relname='notoast_char2';	-- null
create table notoast_varchar1(a varchar2(1 char));
select reltoastrelid from pg_class where relname='notoast_varchar1';	-- null
create table notoast_varchar2(a varchar2(1 byte));
select reltoastrelid from pg_class where relname='notoast_varchar2';	-- null
create table notoast_number(a number(1,1));
select reltoastrelid from pg_class where relname='notoast_number';		-- null
create table notoast_char3(a char(2004 byte));
select reltoastrelid from pg_class where relname='notoast_char3';	-- null
create table toast_char4(a char(2005 byte));

declare
  var int := 0;
begin
  select reltoastrelid into var from pg_class where relname='toast_char4';
  if var > 0 then
    raise notice 'toast';
  else
    raise notice 'no toast';
  end if;
end;
/

create table notoast_varchar3(a varchar2(2004 byte));
select reltoastrelid from pg_class where relname='notoast_varchar3';	-- null
create table toast_varchar4(a varchar2(2005 byte));

declare
  var int := 0;
begin
  select reltoastrelid into var from pg_class where relname='toast_varchar4';
  if var > 0 then
    raise notice 'toast';
  else
    raise notice 'no toast';
  end if;
end;
/

--clean
drop table notoast_char1;
drop table notoast_char2;
drop table notoast_varchar1;
drop table notoast_varchar2;
drop table notoast_number;
drop table test_varchar2_alter;
drop table notoast_char3;
drop table toast_char4;
drop table notoast_varchar3;
drop table toast_varchar4;
/* End - BUG:M0000062 */

/* Begin - BUG#M0000194 */
select to_multi_byte('鲁A');

select to_single_byte('鲁Ａ');

select compose('A' || unistr('\0300'));

select asciistr(compose(unistr('\0041') || unistr('\0300')));

select asciistr(decompose(unistr('\00C0')));
/* End - BUG#M0000194 */

/* Begin - BUG#M0000265 */
create table test_m265(a char(10), b varchar2(10));
insert into test_m265 values('aaa', 'bbb');
select * from test_m265;
select lengthb(a), lengthb(b) from test_m265;	-- 10 3
select lengthb(a), lengthb(cast(a as varchar2(64))) from test_m265;	-- 10 10
select lengthb(b), lengthb(cast(b as char(64))) from test_m265;		-- 3 64

insert into test_m265 values('aaa  ', 'bbb  ');
select a, lengthb(a), b, lengthb(b) from test_m265;	-- 10 3, 10 5

select * from test_m265 where a = 'aaa';								-- 2 line
select * from test_m265 where a = cast('aaa' as char(64));				-- 2 line
select * from test_m265 where a = cast('aaa' as varchar2(64));			-- 0 line
select * from test_m265 where a = cast('aaa       ' as varchar2(64));	-- 2 line
drop table test_m265;
/* End - BUG#M0000265 */

/* Begin - BUG#M0000262 */
CREATE TABLE test_m262 (
  a VARCHAR2(24 char),
  b VARCHAR2(24 byte),
  c char(24 char),
  d char(24 byte)
);

CREATE INDEX idx_test_m262_a ON test_m262(a);
CREATE INDEX idx_test_m262_b ON test_m262(b);
CREATE INDEX idx_test_m262_c ON test_m262(c);
CREATE INDEX idx_test_m262_d ON test_m262(d);

INSERT INTO test_m262 (a, b, c, d)
SELECT 
  'a' || RANDOM(),
  'b' || RANDOM(),
  'c' || RANDOM(),
  'd' || RANDOM()
FROM generate_series (1,100000);
insert into test_m262 values('testa', 'testb', 'testc', 'testd');
VACUUM ANALYZE test_m262;

-- varchar2(n char) = varchar2(n char)
SELECT * FROM test_m262 WHERE a = 'testa'::varchar2(64 char);						-- 1 line.
EXPLAIN (costs off) SELECT * FROM test_m262 WHERE a = 'testa'::varchar2(64 char);	-- using index scan.

-- varchar2(n char) = varchar2(n byte)
SELECT * FROM test_m262 WHERE a = 'testa'::varchar2(64 byte);						-- 1 line.
EXPLAIN (costs off) SELECT * FROM test_m262 WHERE a = 'testa'::varchar2(64 byte);	-- using index scan.

-- varchar2(n char) = char(n char)
SELECT * FROM test_m262 WHERE a = 'testa'::char(5 char);							-- 1 line.
EXPLAIN (costs off) SELECT * FROM test_m262 WHERE a = 'testa'::char(5 char);		-- using index scan.

-- varchar2(n char) = char(n char)
SELECT * FROM test_m262 WHERE a = 'testa'::char(6 char);							-- 0 line.
EXPLAIN (costs off) SELECT * FROM test_m262 WHERE a = 'testa'::char(6 char);		-- using index scan.

-- varchar2(n char) = char(n byte)
SELECT * FROM test_m262 WHERE a = 'testa'::char(5 byte);							-- 1 line.
EXPLAIN (costs off) SELECT * FROM test_m262 WHERE a = 'testa'::char(5 byte);		-- using index scan.

-- varchar2(n char) = char(n byte)
SELECT * FROM test_m262 WHERE a = 'testa'::char(6 byte);							-- 0 line.
EXPLAIN (costs off) SELECT * FROM test_m262 WHERE a = 'testa'::char(6 byte);		-- using index scan.

-- varchar2(n byte) = varchar2(n byte)
SELECT * FROM test_m262 WHERE b = 'testb'::varchar2(64 byte);						-- 1 line.
EXPLAIN (costs off) SELECT * FROM test_m262 WHERE b = 'testb'::varchar2(64 byte);	-- using index scan.

-- varchar2(n byte) = varchar2(n char)
SELECT * FROM test_m262 WHERE b = 'testb'::varchar2(64 char);						-- 1 line.
EXPLAIN (costs off) SELECT * FROM test_m262 WHERE b = 'testb'::varchar2(64 char);	-- using index scan.

-- varchar2(n byte) = char(n char)
SELECT * FROM test_m262 WHERE b = 'testb'::char(5 char);							-- 1 line.
EXPLAIN (costs off) SELECT * FROM test_m262 WHERE b = 'testb'::char(5 char);		-- using index scan.

-- varchar2(n byte) = char(n char)
SELECT * FROM test_m262 WHERE b = 'testb'::char(6 char);							-- 0 line.
EXPLAIN (costs off) SELECT * FROM test_m262 WHERE b = 'testb'::char(6 char);		-- using index scan.

-- varchar2(n byte) = char(n byte)
SELECT * FROM test_m262 WHERE b = 'testb'::char(5 byte);							-- 1 line.
EXPLAIN (costs off) SELECT * FROM test_m262 WHERE b = 'testb'::char(5 byte);		-- using index scan.

-- varchar2(n byte) = char(n byte)
SELECT * FROM test_m262 WHERE b = 'testb'::char(6 byte);							-- 0 line.
EXPLAIN (costs off) SELECT * FROM test_m262 WHERE b = 'testb'::char(6 byte);		-- using index scan.


-- char(n char) = char(n char)
SELECT * FROM test_m262 WHERE c = 'testc'::char(8 char);							-- 1 line.
EXPLAIN (costs off) SELECT * FROM test_m262 WHERE c = 'testc'::char(8 char);		-- using index scan.

-- char(n char) = char(n byte)
SELECT * FROM test_m262 WHERE c = 'testc'::char(8 byte);							-- 1 line.
EXPLAIN (costs off) SELECT * FROM test_m262 WHERE c = 'testc'::char(8 byte);		-- using index scan.

-- char(n char) = varchar2(n char)
SELECT * FROM test_m262 WHERE c = 'testc'::varchar2(64 char);						-- 0 line.
EXPLAIN (costs off) SELECT * FROM test_m262 WHERE c = 'testc'::varchar2(64 char);	-- using index scan.

-- char(n char) = varchar2(n char)
SELECT * FROM test_m262 WHERE c = 'testc                   '::varchar2(64 char);						-- 1 line.
EXPLAIN (costs off) SELECT * FROM test_m262 WHERE c = 'testc                   '::varchar2(64 char);	-- using index scan.

-- char(n char) = varchar2(n byte)
SELECT * FROM test_m262 WHERE c = 'testc'::varchar2(64 byte);						-- 0 line.
EXPLAIN (costs off) SELECT * FROM test_m262 WHERE c = 'testc'::varchar2(64 byte);	-- using index scan.

-- char(n char) = varchar2(n byte)
SELECT * FROM test_m262 WHERE c = 'testc                   '::varchar2(64 byte);						-- 1 line.
EXPLAIN (costs off) SELECT * FROM test_m262 WHERE c = 'testc                   '::varchar2(64 byte);	-- using index scan.


-- char(n byte) = char(n byte)
SELECT * FROM test_m262 WHERE d = 'testd'::char(8 byte);							-- 1 line.
EXPLAIN (costs off) SELECT * FROM test_m262 WHERE d = 'testd'::char(8 byte);		-- using index scan.

-- char(n byte) = char(n char)
SELECT * FROM test_m262 WHERE d = 'testd'::char(8 char);							-- 1 line.
EXPLAIN (costs off) SELECT * FROM test_m262 WHERE d = 'testd'::char(8 char);		-- using index scan.

-- char(n byte) = varchar2(n char)
SELECT * FROM test_m262 WHERE d = 'testd'::varchar2(64 char);						-- 0 line.
EXPLAIN (costs off) SELECT * FROM test_m262 WHERE d = 'testd'::varchar2(64 char);	-- using index scan.

-- char(n byte) = varchar2(n char)
SELECT * FROM test_m262 WHERE d = 'testd                   '::varchar2(64 char);						-- 1 line.
EXPLAIN (costs off) SELECT * FROM test_m262 WHERE d = 'testd                   '::varchar2(64 char);	-- using index scan.

-- char(n byte) = varchar2(n byte)
SELECT * FROM test_m262 WHERE d = 'testd'::varchar2(64 byte);						-- 0 line.
EXPLAIN (costs off) SELECT * FROM test_m262 WHERE d = 'testd'::varchar2(64 byte);	-- using index scan.

-- char(n byte) = varchar2(n byte)
SELECT * FROM test_m262 WHERE d = 'testd                   '::varchar2(64 byte);						-- 1 line.
EXPLAIN (costs off) SELECT * FROM test_m262 WHERE d = 'testd                   '::varchar2(64 byte);	-- using index scan.

DROP TABLE test_m262;

-- test for hash index.
CREATE TABLE test_m262 (
  a VARCHAR2(24 char),
  b VARCHAR2(24 byte),
  c char(24 char),
  d char(24 byte)
);

CREATE INDEX idx_test_m262_a ON test_m262 using hash(a);
CREATE INDEX idx_test_m262_b ON test_m262 using hash(b);
CREATE INDEX idx_test_m262_c ON test_m262 using hash(c);
CREATE INDEX idx_test_m262_d ON test_m262 using hash(d);

INSERT INTO test_m262 (a, b, c, d)
SELECT 
  'a' || RANDOM(),
  'b' || RANDOM(),
  'c' || RANDOM(),
  'd' || RANDOM()
FROM generate_series (1,100000);
insert into test_m262 values('testa', 'testb', 'testc', 'testd');
VACUUM ANALYZE test_m262;

-- varchar2(n char) = varchar2(n char)
SELECT * FROM test_m262 WHERE a = 'testa'::varchar2(64 char);						-- 1 line.
EXPLAIN (costs off) SELECT * FROM test_m262 WHERE a = 'testa'::varchar2(64 char);	-- using index scan.

-- varchar2(n char) = varchar2(n byte)
SELECT * FROM test_m262 WHERE a = 'testa'::varchar2(64 byte);						-- 1 line.
EXPLAIN (costs off) SELECT * FROM test_m262 WHERE a = 'testa'::varchar2(64 byte);	-- using index scan.

-- varchar2(n char) = char(n char)
SELECT * FROM test_m262 WHERE a = 'testa'::char(5 char);							-- 1 line.
EXPLAIN (costs off) SELECT * FROM test_m262 WHERE a = 'testa'::char(5 char);		-- using index scan.

-- varchar2(n char) = char(n char)
SELECT * FROM test_m262 WHERE a = 'testa'::char(6 char);							-- 0 line.
EXPLAIN (costs off) SELECT * FROM test_m262 WHERE a = 'testa'::char(6 char);		-- using index scan.

-- varchar2(n char) = char(n byte)
SELECT * FROM test_m262 WHERE a = 'testa'::char(5 byte);							-- 1 line.
EXPLAIN (costs off) SELECT * FROM test_m262 WHERE a = 'testa'::char(5 byte);		-- using index scan.

-- varchar2(n char) = char(n byte)
SELECT * FROM test_m262 WHERE a = 'testa'::char(6 byte);							-- 0 line.
EXPLAIN (costs off) SELECT * FROM test_m262 WHERE a = 'testa'::char(6 byte);		-- using index scan.

-- varchar2(n byte) = varchar2(n byte)
SELECT * FROM test_m262 WHERE b = 'testb'::varchar2(64 byte);						-- 1 line.
EXPLAIN (costs off) SELECT * FROM test_m262 WHERE b = 'testb'::varchar2(64 byte);	-- using index scan.

-- varchar2(n byte) = varchar2(n char)
SELECT * FROM test_m262 WHERE b = 'testb'::varchar2(64 char);						-- 1 line.
EXPLAIN (costs off) SELECT * FROM test_m262 WHERE b = 'testb'::varchar2(64 char);	-- using index scan.

-- varchar2(n byte) = char(n char)
SELECT * FROM test_m262 WHERE b = 'testb'::char(5 char);							-- 1 line.
EXPLAIN (costs off) SELECT * FROM test_m262 WHERE b = 'testb'::char(5 char);		-- using index scan.

-- varchar2(n byte) = char(n char)
SELECT * FROM test_m262 WHERE b = 'testb'::char(6 char);							-- 0 line.
EXPLAIN (costs off) SELECT * FROM test_m262 WHERE b = 'testb'::char(6 char);		-- using index scan.

-- varchar2(n byte) = char(n byte)
SELECT * FROM test_m262 WHERE b = 'testb'::char(5 byte);							-- 1 line.
EXPLAIN (costs off) SELECT * FROM test_m262 WHERE b = 'testb'::char(5 byte);		-- using index scan.

-- varchar2(n byte) = char(n byte)
SELECT * FROM test_m262 WHERE b = 'testb'::char(6 byte);							-- 0 line.
EXPLAIN (costs off) SELECT * FROM test_m262 WHERE b = 'testb'::char(6 byte);		-- using index scan.


-- char(n char) = char(n char)
SELECT * FROM test_m262 WHERE c = 'testc'::char(8 char);							-- 1 line.
EXPLAIN (costs off) SELECT * FROM test_m262 WHERE c = 'testc'::char(8 char);		-- using index scan.

-- char(n char) = char(n byte)
SELECT * FROM test_m262 WHERE c = 'testc'::char(8 byte);							-- 1 line.
EXPLAIN (costs off) SELECT * FROM test_m262 WHERE c = 'testc'::char(8 byte);		-- using index scan.

-- char(n char) = varchar2(n char)
SELECT * FROM test_m262 WHERE c = 'testc'::varchar2(64 char);						-- 0 line.
EXPLAIN (costs off) SELECT * FROM test_m262 WHERE c = 'testc'::varchar2(64 char);	-- using index scan.

-- char(n char) = varchar2(n char)
SELECT * FROM test_m262 WHERE c = 'testc                   '::varchar2(64 char);						-- 1 line.
EXPLAIN (costs off) SELECT * FROM test_m262 WHERE c = 'testc                   '::varchar2(64 char);	-- using index scan.

-- char(n char) = varchar2(n byte)
SELECT * FROM test_m262 WHERE c = 'testc'::varchar2(64 byte);						-- 0 line.
EXPLAIN (costs off) SELECT * FROM test_m262 WHERE c = 'testc'::varchar2(64 byte);	-- using index scan.

-- char(n char) = varchar2(n byte)
SELECT * FROM test_m262 WHERE c = 'testc                   '::varchar2(64 byte);						-- 1 line.
EXPLAIN (costs off) SELECT * FROM test_m262 WHERE c = 'testc                   '::varchar2(64 byte);	-- using index scan.


-- char(n byte) = char(n byte)
SELECT * FROM test_m262 WHERE d = 'testd'::char(8 byte);							-- 1 line.
EXPLAIN (costs off) SELECT * FROM test_m262 WHERE d = 'testd'::char(8 byte);		-- using index scan.

-- char(n byte) = char(n char)
SELECT * FROM test_m262 WHERE d = 'testd'::char(8 char);							-- 1 line.
EXPLAIN (costs off) SELECT * FROM test_m262 WHERE d = 'testd'::char(8 char);		-- using index scan.

-- char(n byte) = varchar2(n char)
SELECT * FROM test_m262 WHERE d = 'testd'::varchar2(64 char);						-- 0 line.
EXPLAIN (costs off) SELECT * FROM test_m262 WHERE d = 'testd'::varchar2(64 char);	-- using index scan.

-- char(n byte) = varchar2(n char)
SELECT * FROM test_m262 WHERE d = 'testd                   '::varchar2(64 char);						-- 1 line.
EXPLAIN (costs off) SELECT * FROM test_m262 WHERE d = 'testd                   '::varchar2(64 char);	-- using index scan.

-- char(n byte) = varchar2(n byte)
SELECT * FROM test_m262 WHERE d = 'testd'::varchar2(64 byte);						-- 0 line.
EXPLAIN (costs off) SELECT * FROM test_m262 WHERE d = 'testd'::varchar2(64 byte);	-- using index scan.

-- char(n byte) = varchar2(n byte)
SELECT * FROM test_m262 WHERE d = 'testd                   '::varchar2(64 byte);						-- 1 line.
EXPLAIN (costs off) SELECT * FROM test_m262 WHERE d = 'testd                   '::varchar2(64 byte);	-- using index scan.

DROP TABLE test_m262;
/* End - BUG#M0000262 */

/* Begin - BUG#M0000234 */
-- oracharchar vs orachabyte
select 'china'::char(10 char) || 'made'::char(10 char) from dual;
select 'china'::char(10 char) || 'made'::char(10 byte) from dual;
select 'china'::char(10 byte) || 'made'::char(10 byte) from dual;
select 'china'::char(10 byte) || 'made'::char(10 char) from dual;
select length('china'::char(10 byte) || 'made'::char(10 char)) from dual; -- 20

-- oravarcharchar vs oravarcharbyte
select 'china'::varchar2(10 char) || 'made'::varchar2(10 char) from dual;
select 'china'::varchar2(10 char) || 'made'::varchar2(10 byte) from dual;
select 'china'::varchar2(10 byte) || 'made'::varchar2(10 byte) from dual;
select 'china'::varchar2(10 byte) || 'made'::varchar2(10 char) from dual;
select length('china'::varchar2(10 byte) || 'made'::varchar2(10 char)) from dual; -- 9

-- char vs varchar2
select 'china'::char(10 char) || 'made'::varchar2(10 char) from dual;
select 'china'::char(10 char) || 'made'::varchar2(10 byte) from dual;
select 'china'::char(10 byte) || 'made'::varchar2(10 char) from dual;
select 'china'::char(10 byte) || 'made'::varchar2(10 byte) from dual;
select length('china'::char(10 byte) || 'made'::varchar2(10 byte)) from dual; -- 14

-- varchar2 vs char
select 'china'::varchar2(10 char) || 'made'::char(10 char) from dual;
select 'china'::varchar2(10 char) || 'made'::char(10 byte) from dual;
select 'china'::varchar2(10 byte) || 'made'::char(10 char) from dual;
select 'china'::varchar2(10 byte) || 'made'::char(10 byte) from dual;
select length('china'::varchar2(10 byte) || 'made'::char(10 byte)) from dual; -- 15

-- unknown
select 'china'::char(10 char) || 'made' from dual;
select 'china'::char(10 byte) || 'made' from dual;
select 'china'::varchar2(10 char) || 'made' from dual;
select 'china'::varchar2(10 byte) || 'made' from dual;

-- oracle character vs oracle number
select 'china'::char(10 char) || '1949'::number from dual;
select 'china'::char(10 byte) || '1949'::number from dual;
select 'china'::varchar2(10 char) || '1949'::number from dual;
select 'china'::varchar2(10 byte) || '1949'::number from dual;

-- oracle character vs oracle datetime
select 'china'::char(10 char) || date'1949-10-1' from dual;
select 'china'::char(10 byte) || date'1949-10-1' from dual;
select 'china'::varchar2(10 char) || date'1949-10-1' from dual;
select 'china'::varchar2(10 byte) || date'1949-10-1' from dual;
select 'china'::char(10 char) || timestamp'1949-10-1 12:34:56' from dual;
select 'china'::char(10 byte) || timestamp'1949-10-1 12:34:56' from dual;
select 'china'::varchar2(10 char) || timestamp'1949-10-1 12:34:56' from dual;
select 'china'::varchar2(10 byte) || timestamp'1949-10-1 12:34:56' from dual;

-- oracle character vs oracle interval
select 'china'::char(10 char) || interval '1-1' year to month from dual;
select 'china'::char(10 byte) || interval '1-1' year to month from dual;
select 'china'::varchar2(10 char) || interval '1-1' year to month from dual;
select 'china'::varchar2(10 byte) || interval '1-1' year to month from dual;
select 'china'::char(10 char) || interval '1 11:11:11' day to second from dual;
select 'china'::char(10 byte) || interval '1 11:11:11' day to second from dual;
select 'china'::varchar2(10 char) || interval '1 11:11:11' day to second from dual;
select 'china'::varchar2(10 byte) || interval '1 11:11:11' day to second from dual;

-- oracle character vs pg_catalog.int4
select 'china'::char(10 char) || 1949 from dual;
select 'china'::char(10 byte) || 1949 from dual;
select 'china'::varchar2(10 char) || 1949 from dual;
select 'china'::varchar2(10 byte) || 1949 from dual;

-- oracle character vs pg_catalog.numeric
select 'china'::char(10 char) || 19.49::numeric from dual;
select 'china'::char(10 byte) || 19.49::numeric from dual;
select 'china'::varchar2(10 char) || 19.49::numeric from dual;
select 'china'::varchar2(10 byte) || 19.49::numeric from dual;

-- oracle character vs pg_catalog.float4
select 'china'::char(10 char) || 19.49::float4 from dual;
select 'china'::char(10 byte) || 19.49::float4 from dual;
select 'china'::varchar2(10 char) || 19.49::float4 from dual;
select 'china'::varchar2(10 byte) || 19.49::float4 from dual;

-- oracle character vs pg_catalog.float8
select 'china'::char(10 char) || 19.49::float8 from dual;
select 'china'::char(10 byte) || 19.49::float8 from dual;
select 'china'::varchar2(10 char) || 19.49::float8 from dual;
select 'china'::varchar2(10 byte) || 19.49::float8 from dual;

-- oracle character vs pg_catalog.bpchar
select 'china'::char(10 char) || 'made'::pg_catalog.bpchar(10) from dual;
select 'china'::char(10 byte) || 'made'::pg_catalog.bpchar(10) from dual;
select 'china'::varchar2(10 char) || 'made'::pg_catalog.bpchar(10) from dual;
select 'china'::varchar2(10 byte) || 'made'::pg_catalog.bpchar(10) from dual;

-- oracle character vs pg_catalog.varchar
select 'china'::char(10 char) || 'made'::pg_catalog.varchar(10) from dual;
select 'china'::char(10 byte) || 'made'::pg_catalog.varchar(10) from dual;
select 'china'::varchar2(10 char) || 'made'::pg_catalog.varchar(10) from dual;
select 'china'::varchar2(10 byte) || 'made'::pg_catalog.varchar(10) from dual;

-- oracle character vs pg_catalog.date
select 'china'::char(10 char) || '1949-10-1'::pg_catalog.date from dual;
select 'china'::char(10 byte) || '1949-10-1'::pg_catalog.date from dual;
select 'china'::varchar2(10 char) || '1949-10-1'::pg_catalog.date from dual;
select 'china'::varchar2(10 byte) || '1949-10-1'::pg_catalog.date from dual;

-- oracle character vs pg_catalog.timestamp
select 'china'::char(10 char) || '1949-10-1'::pg_catalog.timestamp from dual;
select 'china'::char(10 byte) || '1949-10-1'::pg_catalog.timestamp from dual;
select 'china'::varchar2(10 char) || '1949-10-1'::pg_catalog.timestamp from dual;
select 'china'::varchar2(10 byte) || '1949-10-1'::pg_catalog.timestamp from dual;

-- oracle character vs pg_catalog.timestamptz
select 'china'::char(10 char) || '1949-10-1'::pg_catalog.timestamptz from dual;
select 'china'::char(10 byte) || '1949-10-1'::pg_catalog.timestamptz from dual;
select 'china'::varchar2(10 char) || '1949-10-1'::pg_catalog.timestamptz from dual;
select 'china'::varchar2(10 byte) || '1949-10-1'::pg_catalog.timestamptz from dual;

-- oracle character vs pg_catalog.interval
select 'china'::char(10 char) || '10-1'::pg_catalog.interval from dual;
select 'china'::char(10 byte) || '10-1'::pg_catalog.interval from dual;
select 'china'::varchar2(10 char) || '10-1'::pg_catalog.interval from dual;
select 'china'::varchar2(10 byte) || '10-1'::pg_catalog.interval from dual;

-- pg_catalog.text vs oralce char&varchar2
select 'pg-text'::pg_catalog.text || 'oracle'::char(10 char) from dual;
select 'pg-text'::pg_catalog.text || 'oracle'::char(10 byte) from dual;
select 'pg-text'::pg_catalog.text || 'oracle'::varchar2(10 char) from dual;
select 'pg-text'::pg_catalog.text || 'oracle'::varchar2(10 byte) from dual;

-- pg_catalog.text vs oralce number
select 'pg-text'::pg_catalog.text || '3.14'::number from dual;
select 'pg-text'::pg_catalog.text || '3.14'::number from dual;
select 'pg-text'::pg_catalog.text || '3.14'::number from dual;
select 'pg-text'::pg_catalog.text || '3.14'::number from dual;

-- pg_catalog.text vs oralce binary_float
select 'pg-text'::pg_catalog.text || '3.14'::binary_float from dual;
select 'pg-text'::pg_catalog.text || '3.14'::binary_float from dual;
select 'pg-text'::pg_catalog.text || '3.14'::binary_float from dual;
select 'pg-text'::pg_catalog.text || '3.14'::binary_float from dual;

-- pg_catalog.text vs oralce binary_double
select 'pg-text'::pg_catalog.text || '3.14'::binary_double from dual;
select 'pg-text'::pg_catalog.text || '3.14'::binary_double from dual;
select 'pg-text'::pg_catalog.text || '3.14'::binary_double from dual;
select 'pg-text'::pg_catalog.text || '3.14'::binary_double from dual;

-- pg_catalog.text vs oralce date/timetamp
select 'pg-text'::pg_catalog.text || date'1949-10-1' from dual;
select 'pg-text'::pg_catalog.text || timestamp'1949-10-1 11:11:11' from dual;
select 'pg-text'::pg_catalog.text || timestamp'1949-10-1 11:11:11'::sys.oratimestamptz from dual;
select 'pg-text'::pg_catalog.text || timestamp'1949-10-1 11:11:11'::sys.oratimestampltz from dual;

-- pg_catalog.text vs oralce yminterval/dsinterval
select 'pg-text'::pg_catalog.text || interval '11-11' year to month from dual;
select 'pg-text'::pg_catalog.text || interval '1 11:11:11' day to second from dual;
/* End - BUG#M0000234 */

