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


-- varchar2 and nvarchar2 without specify typmod should issue an error
-- when used in CREATE TABLE/ALTER TABLE, but it is ok for other stmt,
-- for example used in CREATE FUNCTION stmt.
create table test_varchar2(a varchar2);		-- error
create table test_nvarchar2(a nvarchar2);	-- error
create table test_varchar2_alter(a int);
alter table test_varchar2_alter alter column a type varchar2;	-- error
alter table test_varchar2_alter alter column a type nvarchar2;	-- error
alter table test_varchar2_alter alter column a type varchar2(10);	-- ok
\d test_varchar2_alter

alter table test_varchar2_alter alter column a type nvarchar2(20);	-- ok
\d test_varchar2_alter

alter table test_varchar2_alter add column b varchar2;	-- error
alter table test_varchar2_alter add column c nvarchar2;	-- error
alter table test_varchar2_alter add column b varchar2(10);	-- ok
alter table test_varchar2_alter add column c nvarchar2(20);	-- ok
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
