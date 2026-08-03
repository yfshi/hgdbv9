CREATE TABLE mysql_binary_test (s varchar(20) collate utf8_general_ci);

INSERT INTO mysql_binary_test VALUES ('a'), ('A'), ('abc'), ('ABC'), ('a_c'), ('a%c');

set mysql.use_mysql_bit_literal = on;

set sql_mode = 'REAL_AS_FLOAT,PIPES_AS_CONCAT,ANSI_QUOTES,ONLY_FULL_GROUP_BY,STRICT_TRANS_TABLES,NO_ZERO_DATE,ERROR_FOR_DIVISION_BY_ZERO,NO_AUTO_CREATE_USER,NO_ENGINE_SUBSTITUTION,NO_ZERO_IN_DATE';

show sql_mode;

SELECT s FROM mysql_binary_test
WHERE s = _binary 'abc';
-- MySQL 5.7 expected: abc, ABC

SELECT s FROM mysql_binary_test
WHERE s = _binary 'ABC';
-- MySQL 5.7 expected: abc, ABC

SELECT s FROM mysql_binary_test
WHERE s <> _binary 'abc';
-- MySQL 5.7 expected: a, A, a%c, a_c

------***************************
SELECT s FROM mysql_binary_test
WHERE s LIKE _binary 'a_c';
-- MySQL 5.7 expected: a%c, abc, ABC, a_c

------***************************
SELECT s FROM mysql_binary_test
WHERE s LIKE _binary 'A%';
-- MySQL 5.7 expected: a, A, a%c, abc, ABC, a_c

SELECT s FROM mysql_binary_test
WHERE s IN (_binary 'abc', 'zzz');
-- MySQL 5.7 expected: abc, ABC

SELECT s FROM mysql_binary_test
WHERE s = ANY (ARRAY[_binary 'abc', 'zzz']);
-- MySQL 5.7 expected: abc, ABC

SELECT s FROM mysql_binary_test
WHERE s <> ALL (ARRAY[_binary 'abc', 'zzz']);
-- MySQL 5.7 expected: a, A, a%c, a_c

SELECT s FROM mysql_binary_test
WHERE s REGEXP _binary '^[a-z]+$';
-- MySQL 5.7 expected: a, A, abc, ABC

SELECT 'A' REGEXP '^[a-z]+$' AS regexp_default_case_insensitive;
-- MySQL 5.7 expected: 1

SELECT 'A' REGEXP ('^[a-z]+$' COLLATE "C") AS regexp_collate_c_still_case_insensitive;
-- MySQL 5.7 expected: 1


--pg 1 case-insenctive
SELECT 'A' collate utf8_general_ci  ~* _binary '^[a-z]+$' AS regexp_binary_case_sensitive;
--pg 0
SELECT 'A'   ~** _binary '^[a-z]+$' AS regexp_binary_case_sensitive;

--pg 0 case-senctive
SELECT 'A' collate utf8_general_ci  ~ _binary '^[a-z]+$' AS regexp_binary_case_sensitive;
--pg 0 case-senctive
SELECT 'A'   ~ _binary '^[a-z]+$' AS regexp_binary_case_sensitive;

SELECT 'A' collate utf8_general_ci  REGEXP _binary '^[a-z]+$' AS regexp_binary_case_sensitive;
-- MySQL 5.7 expected: 0
SELECT 'A' REGEXP _binary '^[a-z]+$' AS regexp_binary_case_sensitive;
-- MySQL 5.7 expected: 0


------***************************
SELECT 'A' REGEXP (_binary '^[a-z]+$') AS regexp_binary_collate_c_still_case_sensitive;
-- MySQL 5.7 expected: 0

SELECT 'A' REGEXP (_binary '^[a-z]+$' COLLATE "C") AS regexp_binary_collate_c_still_case_sensitive;
-- MySQL 5.7 expected: 0


SELECT _binary X'41' = 'A' AS binary_hex_literal_equals_text;
SELECT _binary 0x41 = 'A' AS binary_hex_literal_equals_text;
-- MySQL 5.7 expected: 1

SELECT _binary B'01000001' = 'A' AS binary_bit_literal_equals_text;
-- MySQL 5.7 expected: 0

SELECT 'A' REGEXP _binary X'5e5b612d7a5d2b24' AS regexp_binary_hex_literal;
SELECT 'A' REGEXP _binary 0x5e5b612d7a5d2b24 AS regexp_binary_hex_literal;
-- MySQL 5.7 expected: 0

SELECT 'A' REGEXP _binary B'0101111001011011011000010010110101111010010111010010101100100100' AS regexp_binary_bit_literal;
-- MySQL 5.7 expected: 0

SELECT s FROM mysql_binary_test
WHERE s NOT REGEXP _binary '^[a-z]+$';
-- MySQL 5.7 expected: a%c, a_c

SELECT s FROM mysql_binary_test
WHERE (s = _binary 'abc') OR s = 'ABC';
-- MySQL 5.7 expected: abc, ABC


------***************************
SELECT s FROM mysql_binary_test
WHERE NOT s LIKE _binary 'A%';
-- MySQL 5.7 expected: (empty set)

SELECT _binary 'x' COLLATE "C" = 'x' AS collated_binary_literal;
-- MySQL 5.7 expected: 1

DROP TABLE mysql_binary_test;





-- MySQL _binary / charset introducer compatibility cases.
-- Expected byte results are shown as hex bytes.
select _BiNary 'abc';          -- ok: bytes 61 62 63
select _binary 'abc';          -- ok: bytes 61 62 63
select _binary 'abd';          -- ok: bytes 61 62 64
select _binary 'abdv';         -- ok: bytes 61 62 64 76
select _binary '6162';         -- ok: bytes 36 31 36 32
select _binary '5c6e';         -- ok: bytes 35 63 36 65

select _binary '\0';           -- ok: bytes 00
select _binary '\Z';           -- ok: bytes 1a
select _binary '\n';           -- ok: bytes 0a
select _binary '\r';           -- ok: bytes 0d
select _binary '\t';           -- ok: bytes 09
select _binary '\b';           -- ok: bytes 08
select _binary '''''';         -- ok: bytes 27 27
select _binary '123\
aa';                           -- ok: bytes 31 32 33 0a 61 61
--select _binary '\'\'';         -- ok: mysql server ok:''


select _binary '\u1';          -- ok: bytes 75 31
select _binary '\x00';         -- ok: bytes 78 30 30

--**********************
set standard_conforming_strings = off;
select _binary '\'';           -- ok: bytes 27
-- select _binary 'abc\';        -- failed ,sql not end.
set standard_conforming_strings = default;
select _binary '\\';           -- ok: bytes 5c
select _binary 'a''b';         -- ok: bytes 61 27 62

select _binary x'6162';        -- ok: bytes 61 62
select _binary X'6162';        -- ok: bytes 61 62
select _binary x'5c6e';        -- ok: bytes 5c 6e
select _binary x'';            -- ok: empty bytes
select _binary x'abc';         -- mysql error: odd number of hex digits in x'...'
select _binary x'g1';          -- mysql error: invalid hex digit
select _binary x'61g2';        -- mysql error: invalid hex digit

select _binary 0x6162;         -- ok: bytes 61 62
select _binary 0X6162;         -- mysql error: not support 0X
select _binary 0xabc;          -- ok: bytes 0a bc
select _binary 0x0abc;         -- ok: bytes 0a bc
select _binary 0x;             -- mysql error: invalid hexadecimal integer
select _binary 0x_;            -- mysql error: invalid hexadecimal integer
select _binary 0x12_;          -- mysql error: invalid hexadecimal integer
select _binary 0xg1;           -- mysql error: invalid hexadecimal integer

select _binary b'01100001';    -- ok: bytes 61
select _binary B'01100001';    -- ok: bytes 61
select _binary b'1';           -- ok: bytes 01
select _binary b'101010111100'; -- ok: bytes 0a bc
select _binary b'';            -- ok: empty bytes


select _binary;                -- mysql error: unterminated MySQL character set introducer
select _binary abc;            -- mysql error: introducer must be followed by a string literal
select _binary 123;            -- mysql error: introducer must be followed by a string literal

select _utf8 'abc';            -- mysql-compatible parser path recognized; current implementation error: _utf8 not supported
select _utf8mb4 'abc';         -- mysql-compatible parser path recognized; current implementation error: _utf8mb4 not supported
select _utf8 x'e4b8ad';         --ok
select _utf8 x'c0af';           --ERROR 2600 (HY000): invalid utf8 character string: "C0AF"
select _utf8 x'ff';             --ERROR 1300 (HY000): Invalid utf8 character string: 'FF'
select _latin1 x'ff';           --ok



create table test_col_type as select _binary 'aaay';
\d test_col_type;
drop table test_col_type;


create table test_col_b as select _binary b'010000001';
\d test_col_b;
drop table test_col_b;

select _binary b'102';         -- mysql error: invalid binary digit
set mysql.use_mysql_bit_literal = default;

select _binary b'102';


set sql_mode = 'NO_BACKSLASH_ESCAPES,REAL_AS_FLOAT,PIPES_AS_CONCAT,ANSI_QUOTES,STRICT_TRANS_TABLES,NO_ZERO_DATE,ERROR_FOR_DIVISION_BY_ZERO,NO_AUTO_CREATE_USER,NO_ENGINE_SUBSTITUTION,NO_ZERO_IN_DATE';

select _binary '\0';           -- ok: bytes 5c 30
select _binary '\Z';           -- ok: bytes 5c 5a
select _binary '\n';           -- ok: bytes 5c 6e
select _binary '\r';           -- ok: bytes 5c 72
select _binary '\t';           -- ok: bytes 5c 74
select _binary '\b';           -- ok: bytes 5c 62
select _binary '\\';           -- ok: bytes 5c 5c

select _binary 'a' COLLATE 'C';
select _binary 'a' COLLATE "C";
-- error ,because _binary current use default collate to achieve, not binary collate.
select cast ('aa' as _bianry) COLLATE 'binary';

-- expected 1
SELECT _binary 'A' REGEXP ('^[a-z]+$' COLLATE 'utf8_general_ci');
-- expected 0
SELECT 'A' REGEXP ('^[a-z]+$' COLLATE 'utf8_bin');
-- expected 0
SELECT _binary 'A' REGEXP ('^[a-z]+$' COLLATE 'utf8_bin');

-- bug#7845
create table users (
            id int primary key auto_increment,
            name varchar(50)  collate utf8_general_ci,
            phone varchar(20),
            email varchar(100)
            );
insert into   users (name, phone, email) values
            ('tony', '13800138000', 'tony@test.com'),
            ('Tony', '13900139000', 'Tony@test.com'),
            ('tony123', '15000150000', 'tony123@test.com'),
            ('tom', '18600186000', 'tom@test.com'),
            ('jerry', '', 'jerry@test.com'),
            ('', '13100131000', ''),
            (NULL, '13200132000', NULL);

-- uncasesencitive
SELECT name FROM users WHERE name REGEXP 'TONY';
-- casesencitive
SELECT name FROM users WHERE name REGEXP binary 'TONY';
SELECT name FROM users WHERE name REGEXP  ('TONY' collate utf8_bin);
-- error
SELECT name FROM users WHERE name REGEXP  ('TONY' collate binary);

set sql_mode = default;