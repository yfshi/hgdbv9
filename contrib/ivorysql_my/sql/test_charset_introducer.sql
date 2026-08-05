-- MySQL character-set introducer test cases.
-- Run in MySQL 5.7+.
-- These cases include full DROP/CREATE/INSERT/SELECT context.
set mysql.use_mysql_bit_literal = on;
set sql_mode = 'NO_BACKSLASH_ESCAPES,REAL_AS_FLOAT,PIPES_AS_CONCAT,ANSI_QUOTES,STRICT_TRANS_TABLES,NO_ZERO_DATE,ERROR_FOR_DIVISION_BY_ZERO,NO_AUTO_CREATE_USER,NO_ENGINE_SUBSTITUTION,NO_ZERO_IN_DATE';

-- 1
select _utf8 x'E4B8AD' = _gbk x'D6D0';
-- 1
select _binary X'D6D0' = _gbk X'D6D0';
-- 1
select _binary X'E4B8AD' = _utf8 '中';
-- 0  --- Warning | 1300 | Invalid gbk character string: 'AD'  
select _binary X'D6D0' = _gbk '中';

-- warning：Invalid gbk character string: 'AD' 
SELECT _gbk '中';

-- warning：Invalid gbk character string: 'AD' 
select _binary X'E4B8AD' = _gbk '中';

-- warning：Invalid gbk character string: 'AD' 
select X'E4B8AD' = _gbk '中';
-- 0
select X'E4B8AD' != _gbk '中';

--  FF
select HEX(_latin1 x'ff');
-- c3bf
select _latin1 x'ff';

-- D6D0
select _gbk x'D6D0';
-- D6D0
select hex(_gbk x'D6D0');
-- E4B8AD
select hex(_gbk '中');
-- Invalid gbk character string: 'AD' ----0
select _utf8 x'E4B8AD' = _gbk '中';

-- 1  a:61
select _utf8'a' = _utf8 x'41' COLLATE utf8_general_ci;
-- 0
select _utf8'a' = _utf8 x'41' COLLATE utf8_bin;

-- 1
select !_gbk 'aaa';
-- 0
select - _gbk 'aaa';

CREATE TABLE test_utf8_introducer (
  c VARCHAR(10) CHARACTER SET utf8
);
INSERT INTO test_utf8_introducer VALUES (_utf8mb4 x'F09F98B2');
INSERT INTO test_utf8_introducer VALUES (_utf8mb4 '😲');
DROP TABLE  test_utf8_introducer;

-- right HCONST
create table  aa as select _utf8 x'E4B8AD';
\d aa;
drop table aa;

create table  aa as select _binary x'E4B8AD';
\d aa;
drop table aa;


-- ----------------------------------------------------------------------
-- 1. _binary '...' keeps the text-literal bytes.
-- ----------------------------------------------------------------------

DROP TABLE IF EXISTS charset_intro_bin_text;
CREATE TABLE charset_intro_bin_text
(
  id INT PRIMARY KEY AUTO_INCREMENT,
  v  VARBINARY(50)
);

INSERT INTO charset_intro_bin_text(v) VALUES
  (_binary 'a'),
  (_binary '6162'),
  (_binary '\0'),
  (_binary '\n'),
  (_binary '\\'),
  (_binary 'a''b');

SELECT id, HEX(v) AS hex_value, LENGTH(v) AS byte_len
FROM charset_intro_bin_text
ORDER BY id;

-- Expected hex:
-- +----+-----------+----------+
-- | id | hex_value | byte_len |
-- +----+-----------+----------+
-- |  1 | 61        |        1 |
-- |  2 | 36313632  |        4 |
-- |  3 | 00        |        1 |
-- |  4 | 0A        |        1 |
-- |  5 | 5C        |        1 |
-- |  6 | 612762    |        3 |
-- +----+-----------+----------+

DROP TABLE charset_intro_bin_text;


-- ----------------------------------------------------------------------
-- 2. _binary x'...' decodes hex text into bytes before storage.
-- ----------------------------------------------------------------------

DROP TABLE IF EXISTS charset_intro_bin_hex;
CREATE TABLE charset_intro_bin_hex
(
  id INT PRIMARY KEY AUTO_INCREMENT,
  v  VARBINARY(50)
);

INSERT INTO charset_intro_bin_hex(v) VALUES
  (_binary x'61'),
  (_binary x'6162'),
  (_binary x'D6D0'),
  (_binary x'E4B8AD'),
  (_binary x'');

SELECT id, HEX(v) AS hex_value, LENGTH(v) AS byte_len
FROM charset_intro_bin_hex
ORDER BY id;

-- Expected hex:
-- +----+-----------+----------+
-- | id | hex_value | byte_len |
-- +----+-----------+----------+
-- |  1 | 61        |        1 |
-- |  2 | 6162      |        2 |
-- |  3 | D6D0      |        2 |
-- |  4 | E4B8AD    |        3 |
-- |  5 |           |        0 |
-- +----+-----------+----------+


DROP TABLE charset_intro_bin_hex;


-- ----------------------------------------------------------------------
-- 3. _binary b'...' decodes bit text into bytes before storage.
-- ----------------------------------------------------------------------

DROP TABLE IF EXISTS charset_intro_bin_bit;
CREATE TABLE charset_intro_bin_bit
(
  id INT PRIMARY KEY AUTO_INCREMENT,
  v  VARBINARY(50)
);

INSERT INTO charset_intro_bin_bit(v) VALUES
  (_binary b'01100001'),
  (_binary b'0110000101100010'),
  (_binary b'1'),
  (_binary b'101010111100'),
  (_binary b'');

SELECT id, HEX(v) AS hex_value, LENGTH(v) AS byte_len
FROM charset_intro_bin_bit
ORDER BY id;

-- Expected hex:
-- +----+-----------+----------+
-- | id | hex_value | byte_len |
-- +----+-----------+----------+
-- |  1 | 61        |        1 |
-- |  2 | 6162      |        2 |
-- |  3 | 01        |        1 |
-- |  4 | 0ABC      |        2 |
-- |  5 |           |        0 |
-- +----+-----------+----------+


DROP TABLE charset_intro_bin_bit;


-- ----------------------------------------------------------------------
-- 4. _gbk x'...' checks decoded bytes as GBK.
--    D6D0 is GBK bytes for the Chinese character "中".
-- ----------------------------------------------------------------------

DROP TABLE IF EXISTS charset_intro_gbk_hex_bincol;
CREATE TABLE charset_intro_gbk_hex_bincol
(
  id INT PRIMARY KEY AUTO_INCREMENT,
  v  VARBINARY(50)
);

INSERT INTO charset_intro_gbk_hex_bincol(v) VALUES
  (_gbk x'D6D0'),
  (_gbk x'4142');

SELECT id, HEX(v) AS hex_value, LENGTH(v) AS byte_len
FROM charset_intro_gbk_hex_bincol
ORDER BY id;

-- Expected hex:
-- +----+-----------+----------+
-- | id | hex_value | byte_len |
-- +----+-----------+----------+
-- |  1 | D6D0      |        2 |
-- |  2 | 4142      |        2 |
-- +----+-----------+----------+

DROP TABLE charset_intro_gbk_hex_bincol;


-- ----------------------------------------------------------------------
-- 5. _gbk x'D6D0' inserted into text columns can be converted by column
--    storage. Compare VARBINARY and UTF8 text targets.
-- ----------------------------------------------------------------------

DROP TABLE IF EXISTS charset_intro_gbk_store_compare;
CREATE TABLE charset_intro_gbk_store_compare
(
  id INT PRIMARY KEY AUTO_INCREMENT,
  vb VARBINARY(50) ,
  vc VARCHAR(50) CHARACTER SET utf8 ,
  v1  VARCHAR(50)  ,
  v2  VARCHAR(50) CHARACTER SET gb18030   --只做了兼容性实现
);

INSERT INTO charset_intro_gbk_store_compare(vb, vc, v1,v2) VALUES
  (_gbk x'D6D0', _gbk x'D6D0',_gbk x'D6D0',_gbk x'D6D0');

SELECT id, HEX(vb) , HEX(vc),  HEX(v1), HEX(v2)
FROM charset_intro_gbk_store_compare;

-- Expected:
-- +----+---------+---------+---------+---------+
-- | id | HEX(vb) | HEX(vc) | HEX(v1) | HEX(v2) |
-- +----+---------+---------+---------+---------+
-- |  1 | D6D0    | E4B8AD  | E4B8AD  | D6D0    |
-- +----+---------+---------+---------+---------+


DROP TABLE charset_intro_gbk_store_compare;


create table tt (vc  VARCHAR(50));
insert into tt (vc) values (_gbk x'D6D0');
insert into tt select _gbk 'D6Do';
insert into tt (vc) values (_gbk x'D6D0' collate gbk_chinese_ci);
select HEX(vc) from tt;
-- +----------+
-- | HEX(vc)  |
-- +----------+
-- | E4B8AD   |
-- | 4436446F |
-- | E4B8AD   |
-- +----------+

drop table tt;



DROP TABLE IF EXISTS charset_intro_gbk_store_compare;
CREATE TABLE charset_intro_gbk_store_compare
(
  id INT PRIMARY KEY AUTO_INCREMENT,
  vb VARBINARY(50) 
);

INSERT INTO charset_intro_gbk_store_compare(vb) VALUES (_gbk '中');
INSERT INTO charset_intro_gbk_store_compare(vb) VALUES (_gbk x'D6D0');
INSERT INTO charset_intro_gbk_store_compare(vb) VALUES (_utf8 x'E4B8AD');

SELECT id, HEX(vb)
FROM charset_intro_gbk_store_compare;

-- Expected: 1 warning: Invalid gbk character string: 'AD' 
-- +----+---------+
-- | id | HEX(vb) |
-- +----+---------+
-- |  1 | E4B8AD  |
-- |  2 | D6D0    |
-- |  3 | E4B8AD  |
-- +----+---------+

DROP TABLE charset_intro_gbk_store_compare;
-- ----------------------------------------------------------------------
-- 6. _gbk '中' depends on client bytes.
--    If the client sends UTF-8 bytes for 中, VARBINARY stores E4B8AD.
--    MySQL treats those bytes as GBK for checking because of _gbk.
-- ----------------------------------------------------------------------

DROP TABLE IF EXISTS charset_intro_gbk_text_client_bytes;
CREATE TABLE charset_intro_gbk_text_client_bytes
(
  id INT PRIMARY KEY AUTO_INCREMENT,
  v  VARCHAR(50)
);

-- ERROR 1406 (22001): Data too long for column 'v' at row 1
INSERT INTO charset_intro_gbk_text_client_bytes(v) VALUES
  (_gbk '中');

DROP TABLE charset_intro_gbk_text_client_bytes;


-- ----------------------------------------------------------------------
-- 7. Collation compatibility.
--    _binary can use COLLATE 'binary' or COLLATE `binary`.
--    Bare COLLATE binary is a syntax error in MySQL because binary is a token.
-- ----------------------------------------------------------------------

DROP TABLE IF EXISTS charset_intro_collate;
CREATE TABLE charset_intro_collate
(
  id INT PRIMARY KEY AUTO_INCREMENT,
  v  VARBINARY(50)
);

INSERT INTO charset_intro_collate(v) VALUES
  (_binary 'a' COLLATE 'binary'),
  (_binary 'a' COLLATE `binary`);

SELECT id, HEX(v) AS hex_value
FROM charset_intro_collate
ORDER BY id;

-- Expected hex:
-- 1: 61
-- 2: 61

-- Expected errors if run separately:
-- SELECT _binary 'a' COLLATE binary;
--   syntax error
-- SELECT _binary 'a' COLLATE utf8_general_ci;
--   COLLATION 'utf8_general_ci' is not valid for CHARACTER SET 'binary'

DROP TABLE charset_intro_collate;


-- ----------------------------------------------------------------------
-- 8. Invalid hex/bit syntax examples. Run separately if you want to verify
--    parser/lexer errors.
-- ----------------------------------------------------------------------

SELECT _binary x'g1';
SELECT _binary x'61g2';
SELECT _binary b'102';


-- ----------------------------------------------------------------------
-- 9. Invalid charset-byte examples. Run separately if you want to verify
--    well-formedness errors for decoded bytes.
-- ----------------------------------------------------------------------

-- _utf8 x'D6D0' treats D6D0 as UTF-8 bytes. They are not valid UTF-8.
SELECT _utf8 x'D6D0';

-- _gbk x'D6' is an incomplete GBK two-byte character in many GBK contexts.
SELECT _gbk x'D6';


-- ----------------------------------------------------------------------
-- 10. Collation/coercibility examples from MySQL func_test-style coverage.
--     These use _koi8r and _latin1 introducers in comparison, STRCMP and LIKE. 
-- ----------------------------------------------------------------------

DROP TABLE IF EXISTS charset_intro_coercibility;
CREATE TABLE charset_intro_coercibility
(
  id INT PRIMARY KEY AUTO_INCREMENT,
  case_name VARCHAR(80),
  result_value VARCHAR(80)
);

INSERT INTO charset_intro_coercibility(case_name, result_value) VALUES
  ('utf8_equal_default', _utf8'a' = _utf8'A'),
  ('utf8_equal_general_ci', _utf8'a' = _utf8'A' COLLATE utf8_general_ci ),
  ('utf8_equal_bin', _utf8'a' = _utf8'A' COLLATE utf8_bin ),
  ('utf8_strcmp_default', STRCMP(_utf8'a', _utf8'A') ),
  ('utf8_strcmp_general_ci', STRCMP(_utf8'a', _utf8'A' COLLATE utf8_general_ci) ),
  ('utf8_strcmp_bin', STRCMP(_utf8'a', _utf8'A' COLLATE utf8_bin) ),
  ('utf8_like_default', _utf8'a' LIKE _utf8'A' ),
  ('utf8_like_general_ci', _utf8'a' LIKE _utf8'A' COLLATE utf8_general_ci ),
  ('utf8_like_bin', _utf8'a' LIKE _utf8'A' COLLATE utf8_bin );

SELECT id, case_name, result_value
FROM charset_intro_coercibility
ORDER BY id;

-- +----+-------------------------+--------------+
-- | id | case_name               | result_value |
-- +----+-------------------------+--------------+
-- |  1 | utf8_equal_default     | 1            |
-- |  2 | utf8_equal_general_ci  | 1            |
-- |  3 | utf8_equal_bin         | 0            |
-- |  4 | utf8_strcmp_default    | 0            |
-- |  5 | utf8_strcmp_general_ci | 0            |
-- |  6 | utf8_strcmp_bin        | 1            |
-- |  7 | utf8_like_default      | 1            |
-- |  8 | utf8_like_general_ci   | 1            |
-- |  9 | utf8_like_bin          | 0            |
-- +----+-------------------------+--------------+
-- 9 rows in set (0.00 sec)


-- Expected characteristics:
-- - utf8_general_ci cases are case-insensitive, so 'a' and 'A' compare equal.
-- - utf8_bin cases are binary/case-sensitive, so 'a' and 'A' differ.
-- - Exact numeric result values follow MySQL STRCMP/comparison semantics.
--

DROP TABLE charset_intro_coercibility;


-- ----------------------------------------------------------------------
-- 11. String function examples from MySQL func_str-style coverage.
--     These verify introducers in POSITION, FIND_IN_SET, SUBSTRING_INDEX,
--     BETWEEN and IN.
-- ----------------------------------------------------------------------
-- 1
-- _latin1 'B' -> 0x42   _latin1 'a' -> 0x61    0x42 >= 0x61 collate is casesensitive  so return 0/f
select _latin1'B' >= _latin1'a';
-- 1
select _latin1'B' <= _latin1'c';

DROP TABLE IF EXISTS charset_intro_string_funcs;
CREATE TABLE charset_intro_string_funcs
(
  id INT PRIMARY KEY AUTO_INCREMENT,
  case_name VARCHAR(80),
  result_value VARCHAR(120)
);

INSERT INTO charset_intro_string_funcs(case_name, result_value) VALUES
  ('position_latin1_default',
   CAST(POSITION(_latin1'B' IN _latin1'abcd') AS CHAR)),
  ('position_latin1_bin_haystack',
   CAST(POSITION(_latin1'B' IN _latin1'abcd' COLLATE latin1_bin) AS CHAR)),
  ('position_latin1_bin_needle',
   CAST(POSITION(_latin1'B' COLLATE latin1_bin IN _latin1'abcd') AS CHAR)),
  ('find_in_set_latin1',
   CAST(FIND_IN_SET(_latin1'B', _latin1'a,b,c,d') AS CHAR)),
  ('substring_index_latin1',
   SUBSTRING_INDEX(_latin1'abcdabcdabcd', _latin1'd', 2)),
  ('between_latin1_default',
   CAST(_latin1'B' BETWEEN _latin1'a' AND _latin1'c' AS CHAR)),
  ('between_latin1_bin_expr',
   CAST(_latin1'B' COLLATE latin1_bin BETWEEN _latin1'a' AND _latin1'c' AS CHAR)),
  ('in_latin1_default',
   CAST(_latin1'B' IN (_latin1'a', _latin1'b') AS CHAR)),
  ('in_latin1_bin_expr',
   CAST(_latin1'B' COLLATE latin1_bin IN (_latin1'a', _latin1'b') AS CHAR));

INSERT INTO charset_intro_string_funcs(case_name, result_value) VALUES
  ('position_latin1_default',CAST(POSITION(_latin1'B' IN _latin1'abcd') AS CHAR)),
  ('find_in_set_latin1',CAST(FIND_IN_SET(_latin1'B', _latin1'a,b,c,d') AS CHAR)),
  ('substring_index_latin1',SUBSTRING_INDEX(_latin1'abcdabcdabcd', _latin1'd', 2)),
  ('between_latin1_default',CAST(_latin1'B' BETWEEN _latin1'a' AND _latin1'c' AS CHAR)),
  ('in_latin1_default',CAST(_latin1'B' IN (_latin1'a', _latin1'b') AS CHAR));

SELECT id, case_name, result_value
FROM charset_intro_string_funcs
ORDER BY id;

-- +----+-------------------------+--------------+
-- | id | case_name               | result_value |
-- +----+-------------------------+--------------+
-- |  1 | position_latin1_default | 2            |
-- |  2 | find_in_set_latin1      | 2            |
-- |  3 | substring_index_latin1  | abcdabc      |
-- |  4 | between_latin1_default  | 1            |
-- |  5 | in_latin1_default       | 1            |
-- +----+-------------------------+--------------+


-- +----+------------------------------+--------------+
-- | id | case_name                    | result_value |
-- +----+------------------------------+--------------+
-- |  1 | position_latin1_default      | 2            |
-- |  2 | position_latin1_bin_haystack | 0            |
-- |  3 | position_latin1_bin_needle   | 0            |
-- |  4 | find_in_set_latin1           | 2            |
-- |  5 | substring_index_latin1       | abcdabc      |
-- |  6 | between_latin1_default       | 1            |
-- |  7 | between_latin1_bin_expr      | 0            |
-- |  8 | in_latin1_default            | 1            |
-- |  9 | in_latin1_bin_expr           | 0            |
-- +----+------------------------------+--------------+
-- 9 rows in set (0.00 sec)
-- 
-- Expected characteristics:
-- - latin1 default collation is normally case-insensitive for these tests.
-- - latin1_bin makes comparisons case-sensitive where that collation controls
--   the operation.
-- - SUBSTRING_INDEX returns bytes/text equivalent to "abcdabcd".
--
-- Expected errors if run separately:
SELECT POSITION(_latin1'B' IN _latin2'abcd');
--   ERROR 1267: Illegal mix of collations
SELECT _latin2'B' BETWEEN _latin1'a' AND _latin1'b';
--   ERROR 1270: Illegal mix of collations
SELECT _latin1'B' IN (_latin2'a', _latin1'b');
--   ERROR 1270: Illegal mix of collations

DROP TABLE charset_intro_string_funcs;


-- ----------------------------------------------------------------------
-- 12. _ucs2 x'...' examples from MySQL ctype_ucs-style coverage.
--     x'....' is decoded to bytes first, then interpreted as UCS2.
-- ----------------------------------------------------------------------

DROP TABLE IF EXISTS charset_intro_ucs2_hex;
CREATE TABLE charset_intro_ucs2_hex
(
  id INT PRIMARY KEY AUTO_INCREMENT,
  l VARCHAR(40) CHARACTER SET ucs2,
  r VARCHAR(40) CHARACTER SET ucs2
);

-- not surport this syntax, UCS2 not exsit
INSERT INTO charset_intro_ucs2_hex(l, r) VALUES
  (LPAD(_ucs2 x'0420', 10, _ucs2 x'0421'),
   RPAD(_ucs2 X'0420', 10, _ucs2 X'0421')),
  (LPAD(_ucs2 X'0420', 10, _ucs2 X'04210422'),
   RPAD(_ucs2 X'0420', 10, _ucs2 X'04210422'));

SELECT id, HEX(l) AS l_hex, HEX(r) AS r_hex, CHAR_LENGTH(l) AS l_chars,
       CHAR_LENGTH(r) AS r_chars
FROM charset_intro_ucs2_hex
ORDER BY id;


-- +----+------------------------------------------+------------------------------------------+---------+---------+
-- | id | l_hex                                    | r_hex                                    | l_chars | r_chars |
-- +----+------------------------------------------+------------------------------------------+---------+---------+
-- |  1 | 0421042104210421042104210421042104210420 | 0420042104210421042104210421042104210421 |      10 |      10 |
-- |  2 | 0421042204210422042104220421042204210420 | 0420042104220421042204210422042104220421 |      10 |      10 |
-- +----+------------------------------------------+------------------------------------------+---------+---------+
-- 2 rows in set (0.01 sec)

-- Expected:
-- - X'0420' and X'0421' are decoded as two-byte UCS2 code units.
-- - CHAR_LENGTH(l) and CHAR_LENGTH(r) are 10 for both rows.
-- - HEX output is UCS2 byte representation, not text bytes for "0420".

DROP TABLE charset_intro_ucs2_hex;


-- ----------------------------------------------------------------------
-- 13. _ucs2 x'...' in LIKE patterns, with table context.
-- ----------------------------------------------------------------------
--  UCS2 not exsit
DROP TABLE IF EXISTS charset_intro_ucs2_like;
CREATE TABLE charset_intro_ucs2_like
(
  word VARCHAR(20) CHARACTER SET ucs2
) ENGINE=MyISAM CHARACTER SET ucs2 COLLATE ucs2_general_ci;

INSERT INTO charset_intro_ucs2_like(word) VALUES ('cat'), ('car'), ('dog');

SELECT word
FROM charset_intro_ucs2_like
WHERE word LIKE _ucs2 x'00630025'
ORDER BY word;

-- +------+
-- | word |
-- +------+
-- | car  |
-- | cat  |
-- +------+
-- 2 rows in set (0.01 sec)


SELECT word
FROM charset_intro_ucs2_like
WHERE word LIKE _ucs2 x'00630061005F'
ORDER BY word;

-- +------+
-- | word |
-- +------+
-- | car  |
-- | cat  |
-- +------+
-- 2 rows in set (0.01 sec)

-- Expected:
-- - _ucs2 x'00630025' is UCS2 bytes for pattern "c%"; returns car, cat.
-- - _ucs2 x'00630061005F' is UCS2 bytes for pattern "ca_"; returns car, cat.

DROP TABLE charset_intro_ucs2_like;


-- ----------------------------------------------------------------------
-- 14. _latin1 x'...' inserted into a UCS2 column.
--     This mirrors MySQL ctype_ucs bug20536 coverage with full context.
-- ----------------------------------------------------------------------

DROP TABLE IF EXISTS charset_intro_latin1_hex_to_ucs2;
CREATE TABLE charset_intro_latin1_hex_to_ucs2
(
  id BIGINT NOT NULL AUTO_INCREMENT PRIMARY KEY,
  name VARCHAR(255) CHARACTER SET ucs2 NOT NULL
);

INSERT INTO charset_intro_latin1_hex_to_ucs2(id, name) VALUES
  (1, _latin1 x'7465737431'),
  (2, _latin1 x'746573745F32');

SELECT id, HEX(name) AS ucs2_hex, name
FROM charset_intro_latin1_hex_to_ucs2
ORDER BY id;

-- +----+--------------------------+--------+
-- | id | ucs2_hex                 | name   |
-- +----+--------------------------+--------+
-- |  1 | 00740065007300740031     | test1  |
-- |  2 | 0074006500730074005F0032 | test_2 |
-- +----+--------------------------+--------+
-- 2 rows in set (0.00 sec)


-- Expected:
-- - x'7465737431' decodes to latin1 bytes for "test1", then stores in UCS2.
-- - Row 1 HEX(name) is expected to be 00740065007300740031.
-- - Row 2 HEX(name) is expected to be 0074006500730074005F0032.

DROP TABLE charset_intro_latin1_hex_to_ucs2;


-- ----------------------------------------------------------------------
-- 15. _gbk with hex numeric literal and conversion, from ctype_gbk-style
--     coverage. This uses 0xA14041, not x'...', but exercises introducer
--     bytes and charset conversion.
-- ----------------------------------------------------------------------

DROP TABLE IF EXISTS charset_intro_gbk_convert;
CREATE TABLE charset_intro_gbk_convert
(
  id INT PRIMARY KEY AUTO_INCREMENT,
  utf8_hex VARBINARY(100)
);

INSERT INTO charset_intro_gbk_convert(utf8_hex) VALUES
  (UNHEX(HEX(CONVERT(_gbk 0xD6D0 USING utf8))));

SELECT id, HEX(utf8_hex) AS converted_utf8_hex
FROM charset_intro_gbk_convert;

-- +----+--------------------+
-- | id | converted_utf8_hex |
-- +----+--------------------+
-- |  1 | E4B8AD               |
-- +----+--------------------+
-- 1 row in set (0.00 sec)

-- Expected:
-- - MySQL test comment says this should produce 0x003F0041:
--   unassigned GBK double-byte 0xA140 converts to question mark 0x003F,
--   then single-byte 0x41 converts to UCS2 0x0041.
-- - Expected converted_utf8_hex: 003F0041

DROP TABLE charset_intro_gbk_convert;


-- ----------------------------------------------------------------------
-- 16. _binary in bitwise/numeric contexts, from varbinary-style coverage.
-- ----------------------------------------------------------------------

DROP TABLE IF EXISTS charset_intro_binary_ops;
CREATE TABLE charset_intro_binary_ops
(
  id INT PRIMARY KEY AUTO_INCREMENT,
  case_name VARCHAR(80),
  result_value VARCHAR(80)
);

INSERT INTO charset_intro_binary_ops(case_name, result_value) VALUES
  ('binary_12_or_text_12', CAST(_binary '12' | '12' AS CHAR)),
  ('binary_12_or_binary_12', CAST(_binary '12' | _binary '12' AS CHAR)),
  -- ('binary_12_or_hex_01', CAST(_binary '12' | 0x01 AS CHAR)),
  -- ('binary_12_or_int_1', CAST(_binary '12' | 1 AS CHAR)),
  ('binary_12_shift_left_1', CAST(_binary '12' << 1 AS CHAR));

SELECT id, case_name, result_value
FROM charset_intro_binary_ops
ORDER BY id;

-- +----+------------------------+--------------+
-- | id | case_name              | result_value |
-- +----+------------------------+--------------+
-- |  1 | binary_12_or_text_12   | 12           |
-- |  2 | binary_12_or_binary_12 | 12           |
-- |  3 | binary_12_or_hex_01    | 13           |
-- |  4 | binary_12_or_int_1     | 13           |
-- |  5 | binary_12_shift_left_1 | 24           |
-- +----+------------------------+--------------+
-- 5 rows in set (0.00 sec)

-- Expected:
-- - These expressions use MySQL's numeric interpretation of string/hex
--   operands in bitwise contexts.
-- - They are included to verify that _binary introducer does not prevent
--   numeric-context conversion.

DROP TABLE charset_intro_binary_ops;


-- ----------------------------------------------------------------------
-- 17. _binary in NAME_CONST with COLLATE 'binary', from func_misc-style
--     coverage.
-- ----------------------------------------------------------------------

DROP TABLE IF EXISTS charset_intro_name_const;
CREATE TABLE charset_intro_name_const
(
  a DATE
);

INSERT INTO charset_intro_name_const(a) VALUES ('2009-01-09');

SELECT *
FROM charset_intro_name_const
WHERE a = NAME_CONST('reportDate', _binary'2009-01-09');

SELECT *
FROM charset_intro_name_const
WHERE a = NAME_CONST('reportDate', '2009-01-09');

-- +------------+
-- | a          |
-- +------------+
-- | 2009-01-09 |
-- +------------+
-- 1 row in set (0.04 sec)

-- Expected:
-- - Returns the row with a = 2009-01-09.
-- - This verifies _binary text literal plus COLLATE 'binary' in a function.

DROP TABLE charset_intro_name_const;




select _utf8 'abc';             --ok  
select _utf8mb4 'abc';          --ok 
select _utf8 x'e4b8ad';         --ok
select _utf8 x'c0af';           --ERROR 2600 (HY000): invalid utf8 character string: "C0AF"
select _utf8 x'ff';             --ERROR 1300 (HY000): Invalid utf8 character string: 'FF'
select HEX(_latin1 x'ff');           --ok



create table test_col_type as select _binary 'aaay';
\d test_col_type;
drop table test_col_type;

create table test_col_type2 as select _utf8 'aaay';
\d test_col_type2;
drop table test_col_type2;

create table abc as select concat(_gbk 'D6D0', _gbk'D6D0');
\d abc;
drop table abc;


create table abcc as select hex(_gbk x'd6d0');
\d abcc;
drop table abcc;

-- Extracted MySQL 5.7 _charset introducer test cases.
-- Source tree: mysql-server
SET NAMES utf8;
SET sql_mode = 'NO_ENGINE_SUBSTITUTION';
set mysql.create_database_mode="schema";
show mysql.create_database_mode;
DROP DATABASE IF EXISTS charset_intro_extract_57;
CREATE DATABASE charset_intro_extract_57 DEFAULT CHARACTER SET latin1;
USE charset_intro_extract_57;

-- ============================================================================
-- Source: mysql-test/include/func_in.inc:78-82
-- Purpose: IN predicate with _koi8r and _latin1 introducers.

SET NAMES utf8;
DROP TABLE IF EXISTS t_func_in_intro;
CREATE TABLE t_func_in_intro (a CHAR(100)  NOT NULL);
INSERT INTO t_func_in_intro VALUES ('bbbb'),(_koi8r'����'),(_latin1'����');
SELECT a FROM t_func_in_intro
WHERE a IN ('bbbb',_koi8r'����',_latin1'����')
ORDER BY a;
-- +------------------------------+
-- | a                            |
-- +------------------------------+
-- | bbbb                         |
-- | ï¿½ï¿½ï¿½ï¿½                 |
-- | О©╫О©╫О©╫О©╫                 |
-- +------------------------------+
-- 3 rows in set (0.02 sec)

DROP TABLE t_func_in_intro;


-- ============================================================================
-- Source: mysql-test/include/ctype_german.inc:13-55
-- Purpose: _latin1 hex introducers for German ordering/comparison.

DROP TABLE IF EXISTS t_ctype_german_intro;
CREATE TABLE t_ctype_german_intro ( s1 text collate utf8_general_ci);
\d t_ctype_german_intro;
DELETE FROM t_ctype_german_intro;

INSERT INTO t_ctype_german_intro VALUES ('ud'),('uf');
INSERT INTO t_ctype_german_intro VALUES ('od'),('of');
INSERT INTO t_ctype_german_intro VALUES ('e');
INSERT INTO t_ctype_german_intro VALUES ('ad'),('af');
INSERT INTO t_ctype_german_intro VALUES ('a'),('ae'),(_latin1 0xE4);
INSERT INTO t_ctype_german_intro VALUES ('o'),('oe'),(_latin1 0xF6);
INSERT INTO t_ctype_german_intro VALUES ('s'),('ss'),(_latin1 0xDF);
INSERT INTO t_ctype_german_intro VALUES ('u'),('ue'),(_latin1 0xFC);
INSERT INTO t_ctype_german_intro VALUES (_latin1 0xE6), (_latin1 0xC6);
INSERT INTO t_ctype_german_intro VALUES (_latin1 0x9C), (_latin1 0x8C);

SELECT s1, HEX(s1) FROM t_ctype_german_intro ORDER BY s1;
-- +----+---------+
-- | s1 | HEX(s1) |
-- +----+---------+
-- | a  | 61      |
-- | ä  | C3A4    |
-- | ad | 6164    |
-- | ae | 6165    |
-- | af | 6166    |
-- | e  | 65      |
-- | o  | 6F      |
-- | ö  | C3B6    |
-- | od | 6F64    |
-- | oe | 6F65    |
-- | of | 6F66    |
-- | s  | 73      |
-- | ß  | C39F    |
-- | ss | 7373    |
-- | u  | 75      |
-- | ü  | C3BC    |
-- | ud | 7564    |
-- | ue | 7565    |
-- | uf | 7566    |
-- | Æ  | C386    |
-- | æ  | C3A6    |
-- | Œ  | C592    |
-- | œ  | C593    |
-- +----+---------+
SELECT GROUP_CONCAT(s1 ORDER BY BINARY s1) FROM t_ctype_german_intro GROUP BY s1;
-- +-------------------------------------+
-- | GROUP_CONCAT(s1 ORDER BY BINARY s1) |
-- +-------------------------------------+
-- | a,ä                                 |
-- | ad                                  |
-- | ae                                  |
-- | af                                  |
-- | e                                   |
-- | o,ö                                 |
-- | od                                  |
-- | oe                                  |
-- | of                                  |
-- | s,ß                                 |
-- | ss                                  |
-- | u,ü                                 |
-- | ud                                  |
-- | ue                                  |
-- | uf                                  |
-- | Æ,æ                                 |
-- | Œ,œ                                 |
-- +-------------------------------------+

SELECT s1, HEX(s1), HEX(WEIGHT_STRING(s1))
FROM t_ctype_german_intro
ORDER BY s1, BINARY(s1);
-- ----+---------+------------------------+
-- | s1 | HEX(s1) | HEX(WEIGHT_STRING(s1)) |
-- +----+---------+------------------------+
-- | a  | 61      | 0041                   |
-- | ä  | C3A4    | 0041                   |
-- | ad | 6164    | 00410044               |
-- | ae | 6165    | 00410045               |
-- | af | 6166    | 00410046               |
-- | e  | 65      | 0045                   |
-- | o  | 6F      | 004F                   |
-- | ö  | C3B6    | 004F                   |
-- | od | 6F64    | 004F0044               |
-- | oe | 6F65    | 004F0045               |
-- | of | 6F66    | 004F0046               |
-- | s  | 73      | 0053                   |
-- | ß  | C39F    | 0053                   |
-- | ss | 7373    | 00530053               |
-- | u  | 75      | 0055                   |
-- | ü  | C3BC    | 0055                   |
-- | ud | 7564    | 00550044               |
-- | ue | 7565    | 00550045               |
-- | uf | 7566    | 00550046               |
-- | Æ  | C386    | 00C6                   |
-- | æ  | C3A6    | 00C6                   |
-- | Œ  | C592    | 0152                   |
-- | œ  | C593    | 0152                   |
-- +----+---------+------------------------+

SELECT s1, HEX(s1)
FROM t_ctype_german_intro
WHERE s1='ae'
ORDER BY s1, BINARY(s1);
-- +----+---------+
-- | s1 | HEX(s1) |
-- +----+---------+
-- | ae | 6165    |
-- +----+---------+
DROP TABLE t_ctype_german_intro;


-- ============================================================================
-- Source: mysql-test/t/union.test:647-680
-- Purpose: UNION with _latin1 introducers and latin1_bin COLLATE.
-- ***
DROP TABLE IF EXISTS t_union_intro;
CREATE TABLE t_union_intro AS
(SELECT _latin1'test' collate utf8_general_ci) UNION
(SELECT _latin1'TEST' collate utf8_general_ci) UNION
(SELECT _latin1'TeST' collate utf8_general_ci);
SHOW CREATE TABLE t_union_intro;
SELECT COUNT(*) FROM t_union_intro;
-- +----------+
-- | COUNT(*) |
-- +----------+
-- |        1 |
-- +----------+

DROP TABLE IF EXISTS t_union_intro;
CREATE TABLE t_union_intro AS
(SELECT _latin1'test') UNION
(SELECT _latin1'TEST') UNION
(SELECT _latin1'TeST' );
SHOW CREATE TABLE t_union_intro;
SELECT COUNT(*) FROM t_union_intro;
-- +----------+
-- | COUNT(*) |
-- +----------+
-- |        1 |
-- +----------+

DROP TABLE t_union_intro;

-- ============================================================================
-- Source: mysql-test/t/union.test:954-958
-- Purpose: UNION aggregation between _utf8 and _latin1.

SELECT _utf8'12' UNION SELECT _latin1'12345';
-- -------+
-- | 12    |
-- +-------+
-- | 12    |
-- | 12345 |
-- +-------+


-- ============================================================================
-- Source: mysql-test/t/ctype_collate.test:10-72, 78-105, 129-164
-- Purpose: _latin1 introducers inserted into latin1 column and COLLATE checks.

DROP TABLE IF EXISTS t_ctype_collate_intro;
CREATE TABLE t_ctype_collate_intro (
  latin1_f CHAR(32) CHARACTER SET latin1 NOT NULL collate utf8_general_ci
);

INSERT INTO t_ctype_collate_intro (latin1_f) VALUES (_latin1'A');
INSERT INTO t_ctype_collate_intro (latin1_f) VALUES (_latin1'a');
INSERT INTO t_ctype_collate_intro (latin1_f) VALUES (_latin1'AD');
INSERT INTO t_ctype_collate_intro (latin1_f) VALUES (_latin1'ad');
INSERT INTO t_ctype_collate_intro (latin1_f) VALUES (_latin1'AE');
INSERT INTO t_ctype_collate_intro (latin1_f) VALUES (_latin1'ae');
INSERT INTO t_ctype_collate_intro (latin1_f) VALUES (_latin1'AF');
INSERT INTO t_ctype_collate_intro (latin1_f) VALUES (_latin1'af');
INSERT INTO t_ctype_collate_intro (latin1_f) VALUES (_latin1'B');
INSERT INTO t_ctype_collate_intro (latin1_f) VALUES (_latin1'b');
INSERT INTO t_ctype_collate_intro (latin1_f) VALUES (_latin1'U');
INSERT INTO t_ctype_collate_intro (latin1_f) VALUES (_latin1'u');
INSERT INTO t_ctype_collate_intro (latin1_f) VALUES (_latin1'UE');
INSERT INTO t_ctype_collate_intro (latin1_f) VALUES (_latin1'ue');
INSERT INTO t_ctype_collate_intro (latin1_f) VALUES (_latin1'SS');
INSERT INTO t_ctype_collate_intro (latin1_f) VALUES (_latin1'ss');
INSERT INTO t_ctype_collate_intro (latin1_f) VALUES (_latin1'Y');
INSERT INTO t_ctype_collate_intro (latin1_f) VALUES (_latin1'y');
INSERT INTO t_ctype_collate_intro (latin1_f) VALUES (_latin1'Z');
INSERT INTO t_ctype_collate_intro (latin1_f) VALUES (_latin1'z');

SELECT latin1_f FROM t_ctype_collate_intro ORDER BY latin1_f, HEX(latin1_f);
-- +----------+
-- | latin1_f |
-- +----------+
-- | A        |
-- | a        |
-- | AD       |
-- | ad       |
-- | AE       |
-- | ae       |
-- | AF       |
-- | af       |
-- | B        |
-- | b        |
-- | SS       |
-- | ss       |
-- | U        |
-- | u        |
-- | UE       |
-- | ue       |
-- | Y        |
-- | y        |
-- | Z        |
-- | z        |
-- +----------+
-- 20 rows in set (0.00 sec)


-- ============================================================================
-- Source: mysql-test/t/ctype_many.test:14-233
-- ***
SET CHARACTER SET latin1;
DROP TABLE IF EXISTS t_ctype_many_intro;
CREATE TABLE t_ctype_many_intro (
  comment CHAR(32)  NOT NULL collate utf8_general_ci,
  koi8_ru_f CHAR(32) NOT NULL DEFAULT '' collate utf8_general_ci
);


INSERT INTO t_ctype_many_intro (koi8_ru_f,comment) VALUES ('a','LAT SMALL A');
INSERT INTO t_ctype_many_intro (koi8_ru_f,comment) VALUES ('b','LAT SMALL B');
INSERT INTO t_ctype_many_intro (koi8_ru_f,comment) VALUES ('A','LAT CAPIT A');
INSERT INTO t_ctype_many_intro (koi8_ru_f,comment) VALUES ('B','LAT CAPIT B');
-- collate utf8_general_ci,now  need  to add.
INSERT INTO t_ctype_many_intro (koi8_ru_f,comment) VALUES (_koi8r'�','CYR SMALL A' collate utf8_general_ci);
INSERT INTO t_ctype_many_intro (koi8_ru_f,comment) VALUES (_koi8r'�','CYR SMALL BE' collate utf8_general_ci);
INSERT INTO t_ctype_many_intro (koi8_ru_f,comment) VALUES (_koi8r'�','CYR CAPIT A' collate utf8_general_ci);
INSERT INTO t_ctype_many_intro (koi8_ru_f,comment) VALUES (_koi8r'�','CYR CAPIT BE' collate utf8_general_ci);

SET CHARACTER SET utf8;
SELECT koi8_ru_f, MIN(comment), COUNT(*) FROM t_ctype_many_intro GROUP BY 1 order by koi8_ru_f;
-- +-----------+--------------+----------+
-- | koi8_ru_f | MIN(comment) | COUNT(*) |
-- +-----------+--------------+----------+
-- | a         | LAT CAPIT A  |        2 |
-- | b         | LAT CAPIT B  |        2 |
-- | О©╫       | CYR CAPIT A  |        4 |
-- +-----------+--------------+----------+


-- ALTER TABLE t_ctype_many_intro ADD utf8_f CHAR(32) CHARACTER SET utf8 NOT NULL DEFAULT '';
-- UPDATE t_ctype_many_intro SET utf8_f=CONVERT(koi8_ru_f USING utf8);
SET CHARACTER SET koi8r;
SELECT * FROM t_ctype_many_intro;
-- +--------------+-----------+--------+
-- | comment      | koi8_ru_f | utf8_f |
-- +--------------+-----------+--------+
-- | LAT SMALL A  | a         | a      |
-- | LAT SMALL B  | b         | b      |
-- | LAT CAPIT A  | A         | A      |
-- | LAT CAPIT B  | B         | B      |
-- | CYR SMALL A  | �         | �      |
-- | CYR SMALL BE | �         | �      |
-- | CYR CAPIT A  | �         | �      |
-- | CYR CAPIT BE | �         | �      |
-- +--------------+-----------+--------+
-- 8 rows in set (0.00 sec)


SELECT koi8_ru_f, MIN(comment) FROM t_ctype_many_intro GROUP BY 1 order by koi8_ru_f;
-- +-----------+--------------+
-- | koi8_ru_f | MIN(comment) |
-- +-----------+--------------+
-- | a         | LAT CAPIT A  |
-- | b         | LAT CAPIT B  |
-- | �         | CYR CAPIT A  |
-- +-----------+--------------+
-- 3 rows in set (0.00 sec)


SELECT LOWER(koi8_ru_f) FROM t_ctype_many_intro ORDER BY 1 DESC;
-- +------------------+
-- | LOWER(koi8_ru_f) |
-- +------------------+
-- | Ͽ▒                |
-- | Ͽ▒                |
-- | Ͽ▒                |
-- | Ͽ▒                |
-- | b                |
-- | b                |
-- | a                |
-- | a                |
-- +------------------+
-- 8 rows in set (0.00 sec)

DROP TABLE t_ctype_many_intro;

-- ============================================================================
-- Source: mysql-test/include/ctype_utf8mb4.inc:1475-1491
-- Purpose: conversion tests for character set introducers.
-- ***
SET NAMES utf8mb4;
DROP TABLE IF EXISTS t_utf8mb4_intro;
CREATE TABLE t_utf8mb4_intro (a VARCHAR(10) COLLATE utf8_general_ci, b INT) ENGINE=InnoDB;
INSERT INTO t_utf8mb4_intro VALUES ('a',1);
SELECT CONCAT(a, IF(b>10, _utf8mb4'x', _utf8mb4'y' collate utf8_general_ci)) FROM t_utf8mb4_intro;
-- +-----------------------------------------------+
-- | CONCAT(a, IF(b>10, _utf8mb4'x', _utf8mb4'y')) |
-- +-----------------------------------------------+
-- | ay                                            |
-- +-----------------------------------------------+
-- 1 row in set (0.00 sec)

-- Source expected error 1267:
-- SELECT CONCAT(a, IF(b>10, _utf8mb4'æ', _utf8mb4'ß')) FROM t_utf8mb4_intro;
DROP TABLE t_utf8mb4_intro;

SET NAMES utf8mb4;
DROP TABLE IF EXISTS t_utf8mb4_hex_intro;
CREATE TABLE t_utf8mb4_hex_intro (a VARCHAR(10) CHARACTER SET latin1, b INT) ENGINE=InnoDB;
INSERT INTO t_utf8mb4_hex_intro VALUES ('a',1);
SELECT CONCAT(a, IF(b>10, _utf8mb4 0x78, _utf8mb4 0x79)) FROM t_utf8mb4_hex_intro;
-- +---------------------------------------------------+
-- | CONCAT(a, IF(b>10, _utf8mb4 0x78, _utf8mb4 0x79)) |
-- +---------------------------------------------------+
-- | ay                                                |
-- +---------------------------------------------------+
-- 1 row in set (0.00 sec)

-- Source expected error 1267:
-- SELECT CONCAT(a, IF(b>10, _utf8mb4 0xC3A6, _utf8mb4 0xC3AF)) FROM t_utf8mb4_hex_intro;
DROP TABLE t_utf8mb4_hex_intro;

-- ============================================================================
-- Source: mysql-test/extra/rpl_tests/rpl_charset.test:124-135
-- Purpose: BUG#6676, user variable derived from _latin1 introducer.

DROP DATABASE charset_intro_extract_57 CASCADE;

\c postgres
set ivorysql.compatible_mode  = mysql;

CREATE DATABASE db_gbk
WITH
  ENCODING = 'GBK'
  LC_COLLATE = 'C'
  LC_CTYPE = 'C'
  TEMPLATE = template0;

\c db_gbk;
SHOW server_encoding;

set ivorysql.compatible_mode  = mysql;

CREATE TABLE t (
  c varchar(20)
);
-- ERROR 1366 (HY000): Incorrect string value: '\xE2\x98\x83' for column 'c' at row 1
INSERT INTO t VALUES (_utf8 x'E29883');

select HEX(c) from t;
drop table t;

-- select _utf8 x'E29883';
select  HEX(_utf8 x'E29883');

-- error HCONST
create table bb  as select _utf8 x'E29883';
\d bb;
drop table bb;

-- ok
create table bb  as select _binary x'E29883';
\d bb;
drop table bb;

DROP DATABASE db_gbk;


\c postgres
set ivorysql.compatible_mode  = mysql;
SHOW server_encoding;


-- create function alter sql
WITH target_proc AS (
    /* operator implementation functions */
    SELECT DISTINCT o.oprcode AS oid
    FROM pg_catalog.pg_operator o
    WHERE o.oprname IN ('=', '<=>', '<>', '!=', '<', '<=', '>', '>=')
      AND o.oprcode <> 0
      AND (
            o.oprleft  IN ('text'::regtype, 'varchar'::regtype, 'bpchar'::regtype, 'char'::regtype)
         OR o.oprright IN ('text'::regtype, 'varchar'::regtype, 'bpchar'::regtype, 'char'::regtype)
      )

    UNION

    /* MySQL compatible functions */
    SELECT p.oid
    FROM pg_catalog.pg_proc p
    JOIN pg_catalog.pg_namespace n ON n.oid = p.pronamespace
    WHERE n.nspname = 'ivymy'
      AND lower(p.proname::text) IN (
          'strcmp',
          'least', 'greatest', 'locate', 'instr',
          'field', 'find_in_set', 'replace', 'substring_index',
          'trim',
          'concat', 'concat_ws', 'reverse', 'insert',
          'lower', 'lcase', 'upper', 'ucase',
          'left', 'right',
          'substr', 'substring', 'mid',
          'ltrim', 'rtrim',
          'soundex', 'elt', 'make_set', 'repeat',
          'rpad', 'lpad', 'export_set',
          'ifnull', 'if', 'coalesce',
          'text_seq'
      )
)
SELECT DISTINCT format(
           'ALTER FUNCTION %I.%I(%s) EXPAND "convert_encoding";',
           n.nspname,
           p.proname,
           pg_catalog.pg_get_function_identity_arguments(p.oid)
       ) AS alter_sql
FROM target_proc t
JOIN pg_catalog.pg_proc p ON p.oid = t.oid
JOIN pg_catalog.pg_namespace n ON n.oid = p.pronamespace
ORDER BY alter_sql;






-- SELECT *
-- FROM pg_catalog.pg_object
-- WHERE refclassid = 'pg_proc'::regclass
--   AND option->>'convert_encoding' = '1'
-- ORDER BY obj_oid::regprocedure::text;


create table dd as select _gbk 'D6D0' = _gbk 'D6D0' collate utf8_general_ci;
\d dd;
drop table dd;
create table dd as select _gbk 'D6D0' collate utf8_general_ci;
\d dd;
drop table dd;


CREATE TABLE t_func_in_intro (a CHAR(100)  NOT NULL);
insert into t_func_in_intro values ('bb'), (_gbk x'D6D0'), (_utf8 x'E4B8AD');
select * from t_func_in_intro;
select hex(a)  from t_func_in_intro;
SELECT a FROM t_func_in_intro WHERE a IN  ('bb', _gbk x'D6D0' ,_utf8 x'E4B8AD');
SELECT a FROM t_func_in_intro WHERE a IN  ('bb', _gbk x'D6D0' ,_binary x'E4B8AD');
SELECT a FROM t_func_in_intro WHERE a IN  ('bb', _binary x'D6D0' ,_binary x'E4B8AD');


SELECT a FROM t_func_in_intro WHERE a IN  ( x'E4B8AD',  _gbk x'D6D0' );
SELECT a FROM t_func_in_intro WHERE a IN  ( _gbk x'D6D0' );
-- +-----+
-- | a   |
-- +-----+
-- | 中  |
-- | 中  |
-- +-----+

SELECT a FROM t_func_in_intro WHERE a IN  ('bb',  x'D6D0' );
-- +----+
-- | a  |
-- +----+
-- | bb |
-- +----+


-- empty
SELECT a FROM t_func_in_intro WHERE a IN  ( _binary x'D6D0' );
-- empty,   Warning | 1300 | Invalid gbk character string: 'AD'
SELECT a FROM t_func_in_intro WHERE a IN  ( _gbk '中' );





-- Notes:
--   _gbk x'D6D0' is the GBK byte sequence for the character also encoded as
--   UTF-8 x'E4B8AD'.
--   Many cases use HEX(CONVERT(... USING utf8)) so results are stable across
--   terminals and client character sets.

SET NAMES utf8;
SET sql_mode = '';
set mysql.create_database_mode="schema";
show mysql.create_database_mode;
CREATE DATABASE IF NOT EXISTS charset_conv_test DEFAULT CHARACTER SET utf8;
USE charset_conv_test;

DROP TABLE IF EXISTS function_charset_conversion_seed;

CREATE TABLE function_charset_conversion_seed (
  id INT NOT NULL PRIMARY KEY,
  gbk_col VARCHAR(20) CHARACTER SET gbk,
  utf8_col VARCHAR(20) CHARACTER SET utf8,
  gbk_text TEXT CHARACTER SET gbk,
  utf8_text TEXT CHARACTER SET utf8
) DEFAULT CHARACTER SET utf8;

INSERT INTO function_charset_conversion_seed
  (id, gbk_col, utf8_col, gbk_text, utf8_text)
VALUES
  (1, _gbk x'D6D0', _utf8 x'E4B8AD', _gbk x'D6D0', _utf8 x'E4B8AD'),
  (2, _gbk 'abc', _utf8 'abc', _gbk 'a,b,c', _utf8 'a,b,c');

-- 01. Item_bool_func2::fix_length_and_dec
-- SQL surface: = and other two-argument comparison operators.
-- Expected: 1, because both literals represent the same character after
-- comparison collation aggregation.
SELECT _utf8 x'E4B8AD' = _gbk x'D6D0' AS case_01_eq;

-- 02. Item_func_between::fix_length_and_dec
-- SQL surface: BETWEEN / NOT BETWEEN.
-- Expected: 1. The three string arguments are eligible for comparison
-- charset/collation aggregation.
SELECT _gbk 'b' BETWEEN _utf8 'a' AND _utf8 'z' AS case_02_between;

-- 03. Item_func_nullif::fix_length_and_dec
-- SQL surface: NULLIF(expr1, expr2).
-- Expected: 1/NULL, because NULLIF returns NULL when the two string operands compare
-- equal after charset/collation aggregation.
SELECT HEX(NULLIF(_gbk x'D6D0', _utf8 x'E4B8AD')) AS case_03_nullif_is_null;

-- 04. Item_func_case::fix_length_and_dec
-- SQL surface: CASE value WHEN compare_value THEN result ... END.
-- Expected: 'matched'.
SELECT CASE _gbk x'D6D0'
         WHEN _utf8 x'E4B8AD' THEN 'matched'
         ELSE 'not matched'
       END AS case_04_case_expr;

-- 05. Item_func_in::fix_length_and_dec
-- SQL surface: IN / NOT IN.
-- Expected: 1.
SELECT _gbk x'D6D0' IN (_utf8 x'E4B8AD', _utf8 'A') AS case_05_in;

-- 06. Item_func_min_max::fix_length_and_dec
-- SQL surface: scalar LEAST()/GREATEST(), not aggregate MIN()/MAX().
-- Expected: E4B8AD | 42 the query succeeds and returns one string value after string-result
-- plus comparison aggregation. The exact least value can depend on collation.
SELECT
  HEX(CONVERT(LEAST(_gbk x'D6D0', _utf8 x'E4B8AD') USING utf8)) AS case_06_least_hex,
  HEX(CONVERT(GREATEST(_gbk 'A', _utf8 'B') USING utf8)) AS case_06_greatest_hex;

-- 07. Item_func_locate::fix_length_and_dec
-- SQL surface: LOCATE(), POSITION(), INSTR().
-- Expected: all three columns return 1.
SELECT
  LOCATE(_gbk x'D6D0', _utf8 x'E4B8AD') AS case_07_locate,
  POSITION(_gbk x'D6D0' IN _utf8 x'E4B8AD') AS case_07_position,
  INSTR(_utf8 x'E4B8AD', _gbk x'D6D0') AS case_07_instr;

-- 08. Item_func_field::fix_length_and_dec
-- SQL surface: FIELD(expr, value1, value2, ...).
-- Expected: 1.
SELECT FIELD(_gbk x'D6D0', _utf8 x'E4B8AD', _utf8 'A') AS case_08_field;

-- 09. Item_func_find_in_set::fix_length_and_dec
-- SQL surface: FIND_IN_SET(str, strlist).
-- Expected: 1.
SELECT FIND_IN_SET(_gbk x'D6D0',
                   CONCAT(_utf8 x'E4B8AD', _utf8 ',A')) AS case_09_find_in_set;

-- Expected: 0.
SELECT FIND_IN_SET(_gbk x'D6D0', _utf8 'E4B8AD,A') AS case_09_find_in_set;
-- Expected: 2.
SELECT FIND_IN_SET(_gbk 'A', _utf8 'E4B8AD,A') AS case_09_find_in_set;

-- 10. Item_func_concat::fix_length_and_dec
-- SQL surface: CONCAT(arg1, arg2, ...).
-- Expected: E4B8AD41 after display conversion to utf8.
SELECT HEX(CONVERT(CONCAT(_gbk x'D6D0', _utf8 'A') USING utf8)) AS case_10_concat_hex;

-- 11. Item_func_concat_ws::fix_length_and_dec
-- SQL surface: CONCAT_WS(separator, arg1, arg2, ...).
-- Expected: E4B8AD2C41 after display conversion to utf8.
SELECT HEX(CONVERT(CONCAT_WS(_utf8 ',', _gbk x'D6D0', _utf8 'A') USING utf8)) AS case_11_concat_ws_hex;

-- 12. Item_func_reverse::fix_length_and_dec
-- SQL surface: REVERSE(str).
-- Expected: 41E4B8AD after display conversion to utf8.
SELECT HEX(CONVERT(REVERSE(CONCAT(_gbk x'D6D0', _utf8 'A')) USING utf8)) AS case_12_reverse_hex;

-- 13. Item_func_replace::fix_length_and_dec
-- SQL surface: REPLACE(str, from_str, to_str).
-- Expected: 58, because the matching character is replaced by 'X'.
SELECT HEX(CONVERT(REPLACE(_gbk x'D6D0', _utf8 x'E4B8AD', _utf8 'X') USING utf8)) AS case_13_replace_hex;

-- 14. Item_func_insert::fix_length_and_dec
-- SQL surface: INSERT(str, pos, len, newstr).
-- Expected: 41 after display conversion to utf8.
SELECT HEX(CONVERT(INSERT(_gbk x'D6D0', 1, 1, _utf8 'A') USING utf8)) AS case_14_insert_func_hex;

-- 15. Item_func_lower::fix_length_and_dec
-- SQL surface: LOWER() / LCASE().
-- Expected: 'abc' for both functions.
SELECT LOWER(_gbk 'ABC') AS case_15_lower, LCASE(_gbk 'ABC') AS case_15_lcase;

-- 16. Item_func_upper::fix_length_and_dec
-- SQL surface: UPPER() / UCASE().
-- Expected: 'ABC' for both functions.
SELECT UPPER(_gbk 'abc') AS case_16_upper, UCASE(_gbk 'abc') AS case_16_ucase;

-- 17. Item_func_left::fix_length_and_dec
-- SQL surface: LEFT(str, len).
-- Expected: E4B8AD after display conversion to utf8.
SELECT HEX(CONVERT(LEFT(CONCAT(_gbk x'D6D0', _utf8 'A'), 1) USING utf8)) AS case_17_left_hex;

-- 18. Item_func_right::fix_length_and_dec
-- SQL surface: RIGHT(str, len).
-- Expected: 41 after display conversion to utf8.
SELECT HEX(CONVERT(RIGHT(CONCAT(_gbk x'D6D0', _utf8 'A'), 1) USING utf8)) AS case_18_right_hex;

-- 19. Item_func_substr::fix_length_and_dec
-- SQL surface: SUBSTR() / SUBSTRING() / MID().
-- Expected: E4B8AD for all three columns after display conversion to utf8.
SELECT
  HEX(CONVERT(SUBSTR(CONCAT(_gbk x'D6D0', _utf8 'A'), 1, 1) USING utf8)) AS case_19_substr_hex,
  HEX(CONVERT(SUBSTRING(CONCAT(_gbk x'D6D0', _utf8 'A'), 1, 1) USING utf8)) AS case_19_substring_hex,
  HEX(CONVERT(MID(CONCAT(_gbk x'D6D0', _utf8 'A'), 1, 1) USING utf8)) AS case_19_mid_hex;

-- 20. Item_func_substr_index::fix_length_and_dec
-- SQL surface: SUBSTRING_INDEX(str, delim, count).
-- Expected: E4B8AD after display conversion to utf8.
SELECT HEX(CONVERT(SUBSTRING_INDEX(CONCAT(_gbk x'D6D0', _utf8 ',A'), _utf8 ',', 1) USING utf8)) AS case_20_substring_index_hex;

-- 21. Item_func_trim::fix_length_and_dec
-- SQL surface: TRIM(), LTRIM(), RTRIM().
-- Expected:
--   case_21_trim_hex  = E4B8AD
--   case_21_ltrim     = 'abc'
--   case_21_rtrim     = 'abc'
SELECT
  HEX(CONVERT(TRIM(_utf8 'A' FROM CONCAT(_gbk 'AAA', _gbk x'D6D0', _gbk 'AAA')) USING utf8)) AS case_21_trim_hex,
  LTRIM(_gbk '   abc') AS case_21_ltrim,
  RTRIM(_gbk 'abc   ') AS case_21_rtrim;

-- 22. Item_func_soundex::fix_length_and_dec
-- SQL surface: SOUNDEX(str).
-- Expected: S530 for 'Smith'.
SELECT SOUNDEX(_gbk 'Smith') AS case_22_soundex;

-- 23. Item_func_elt::fix_length_and_dec
-- SQL surface: ELT(N, str1, str2, ...).
-- Expected: E4B8AD after display conversion to utf8.
SELECT HEX(CONVERT(ELT(1, _gbk x'D6D0', _utf8 'A') USING utf8)) AS case_23_elt_hex;

-- 24. Item_func_make_set::fix_length_and_dec
-- SQL surface: MAKE_SET(bits, str1, str2, ...).
-- Expected: E4B8AD2C41 after display conversion to utf8.
SELECT HEX(CONVERT(MAKE_SET(3, _gbk x'D6D0', _utf8 'A') USING utf8)) AS case_24_make_set_hex;

-- 25. Item_func_repeat::fix_length_and_dec
-- SQL surface: REPEAT(str, count).
-- Expected: E4B8ADE4B8AD after display conversion to utf8.
SELECT HEX(CONVERT(REPEAT(_gbk x'D6D0', 2) USING utf8)) AS case_25_repeat_hex;

-- 26. Item_func_rpad::fix_length_and_dec
-- SQL surface: RPAD(str, len, padstr).
-- Expected: E4B8AD4141 after display conversion to utf8.
SELECT HEX(CONVERT(RPAD(_gbk x'D6D0', 3, _utf8 'A') USING utf8)) AS case_26_rpad_hex;

-- 27. Item_func_lpad::fix_length_and_dec
-- SQL surface: LPAD(str, len, padstr).
-- Expected: 4141E4B8AD after display conversion to utf8.
SELECT HEX(CONVERT(LPAD(_gbk x'D6D0', 3, _utf8 'A') USING utf8)) AS case_27_lpad_hex;

-- 28. Item_func_export_set::fix_length_and_dec
-- SQL surface: EXPORT_SET(bits, on, off[, separator[, number_of_bits]]).
-- Expected: 59,4E,59 or hex 592C4E2C59.
SELECT HEX(CONVERT(EXPORT_SET(5, _gbk 'Y', _utf8 'N', _utf8 ',', 3) USING utf8)) AS case_28_export_set_hex;

-- 29. Item_xml_str_func::fix_length_and_dec
-- SQL surface: EXTRACTVALUE() / UPDATEXML().
-- Expected:
--   EXTRACTVALUE returns 'A'
--   UPDATEXML returns '<a>B</a>'
SELECT
  EXTRACTVALUE(_utf8 '<a>A</a>', _gbk '/a') AS case_29_extractvalue,
  UPDATEXML(_utf8 '<a>A</a>', _gbk '/a', _utf8 '<a>B</a>') AS case_29_updatexml;

-- 30. Item_func_equal::fix_length_and_dec
-- SQL surface: NULL-safe equality operator <=>.
-- Expected: 1.
SELECT _utf8 x'E4B8AD' <=> _gbk x'D6D0' AS case_30_null_safe_equal;
-- Expected: 0.
SELECT _utf8 x'E4B8AD' <=>  x'D6D0' AS case_30_null_safe_equal;

-- 31. Item_func_ifnull::fix_length_and_dec
-- SQL surface: IFNULL(expr1, expr2).
-- Expected: E4B8AD after display conversion to utf8.
SELECT HEX(CONVERT(IFNULL(NULL, _gbk x'D6D0') USING utf8)) AS case_31_ifnull_hex;

-- 32. Item_func_if::fix_length_and_dec
-- SQL surface: IF(condition, true_expr, false_expr).
-- Expected: E4B8AD after display conversion to utf8.
SELECT HEX(CONVERT(IF(1, _gbk x'D6D0', _utf8 'A') USING utf8)) AS case_32_if_hex;

-- 33. Item_func_coalesce::fix_length_and_dec
-- SQL surface: COALESCE(expr1, expr2, ...).
-- Expected: E4B8AD after display conversion to utf8.
SELECT HEX(CONVERT(COALESCE(NULL, _gbk x'D6D0', _utf8 'A') USING utf8)) AS case_33_coalesce_hex;

-- 34. Item_func_strcmp::fix_length_and_dec
-- SQL surface: STRCMP(expr1, expr2).
-- Expected: 0, because both strings compare equal after charset/collation aggregation.
SELECT STRCMP(_gbk x'D6D0', _utf8 x'E4B8AD') AS case_34_strcmp;

-- Column-value smoke tests using the table created above.
-- Expected:
--   column_eq = 1
--   concat_cols_hex = E4B8ADE4B8AD
SELECT
  gbk_col = utf8_col AS column_eq,
  HEX(CONVERT(CONCAT(gbk_col, utf8_col) USING utf8)) AS concat_cols_hex
FROM function_charset_conversion_seed
WHERE id = 1;

-- a+1
select _utf8 'a+1' collate utf8_general_ci;
-- 0 --->_utf8 'a+1' + ('a' collate utf8_general_ci)
select _utf8 'a+1' + 'a' collate utf8_general_ci;

-- aaa
select _binary 'aaa' collate 'binary';
-- error
select _binary 'aaa' collate 'utf8_bin';

DROP TABLE function_charset_conversion_seed;
