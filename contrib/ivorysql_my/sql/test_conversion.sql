/*
 * test mysql binary type conversion
 */
set mysql.cast_to_char = on;
-- parameter settings
SET ivorysql.compatible_mode TO mysql;

set mysql.mysql_convert = true;

-- clean environment
DROP TABLE IF EXISTS
  pg_bytea_t14,
  my_binary_t13,
  my_longblob_t12,
  my_mediumblob_t11,
  my_tinyblob_t10,
  my_blob_t09,
  my_varbinary_t08,
  my_binary_t07,
  my_longblob_t06,
  my_mediumblob_t05,
  my_tinyblob_t04,
  my_blob_t03,
  my_varbinary_t02,
  my_binary_t01;

CREATE TABLE my_binary_t01     (c1 binary(10));
CREATE TABLE my_varbinary_t02  (c1 varbinary(10));
CREATE TABLE my_blob_t03       (c1 blob);
CREATE TABLE my_tinyblob_t04   (c1 tinyblob);
CREATE TABLE my_mediumblob_t05 (c1 mediumblob);
CREATE TABLE my_longblob_t06   (c1 longblob);

INSERT  INTO my_binary_t01     VALUES ('1234567890'), ('abcd');
INSERT  INTO my_varbinary_t02  VALUES ('1234567890'), ('abcd');
INSERT  INTO my_blob_t03       VALUES ('1234567890'), ('abcd');
INSERT  INTO my_tinyblob_t04   VALUES ('1234567890'), ('abcd');
INSERT  INTO my_mediumblob_t05 VALUES ('1234567890'), ('abcd');
INSERT  INTO my_longblob_t06   VALUES ('1234567890'), ('abcd');

CREATE TABLE my_binary_t07     (c1 binary(10));
CREATE TABLE my_varbinary_t08  (c1 varbinary(10));
CREATE TABLE my_blob_t09       (c1 blob);
CREATE TABLE my_tinyblob_t10   (c1 tinyblob);
CREATE TABLE my_mediumblob_t11 (c1 mediumblob);
CREATE TABLE my_longblob_t12   (c1 longblob);

INSERT  INTO my_binary_t07     VALUES ('1234'), ('abcd'), ('1q2w'), ('q1w2');
INSERT  INTO my_varbinary_t08  VALUES ('1234'), ('abcd'), ('1q2w'), ('q1w2');
INSERT  INTO my_blob_t09       VALUES ('1234'), ('abcd'), ('1q2w'), ('q1w2');
INSERT  INTO my_tinyblob_t10   VALUES ('1234'), ('abcd'), ('1q2w'), ('q1w2');
INSERT  INTO my_mediumblob_t11 VALUES ('1234'), ('abcd'), ('1q2w'), ('q1w2');
INSERT  INTO my_longblob_t12   VALUES ('1234'), ('abcd'), ('1q2w'), ('q1w2');

CREATE TABLE my_binary_t13     (c1 binary(10), c2 varbinary(10), c3 blob, c4 tinyblob, c5 mediumblob, c6 longblob);
INSERT  INTO my_binary_t13     VALUES ('1234', '1234', '1234', '1234', '1234', '1234');

CREATE TABLE pg_bytea_t14      (c1 bytea);
INSERT  INTO pg_bytea_t14      VALUES ('1234'), ('abcd'), ('1q2w'), ('q1w2');

--------------------------------------------------------------------------------
-- use cast "mysql binary" to "mysql other binary"

-- EXPECT: ok
SELECT  cast(c1 as binary)        FROM my_binary_t01;
SELECT convert(c1, binary)        FROM my_binary_t01;
SELECT         c1::binary         FROM my_binary_t01;

-- EXPECT: ok, but warning
SELECT  cast(c1 as binary(0))     FROM my_binary_t01;
SELECT convert(c1, binary(0))     FROM my_binary_t01;
SELECT         c1::binary(0)      FROM my_binary_t01;

-- EXPECT: ok, but warning
SELECT  cast(c1 as binary(1))     FROM my_binary_t01;
SELECT convert(c1, binary(1))     FROM my_binary_t01;
SELECT         c1::binary(1)      FROM my_binary_t01;

-- EXPECT: ok
SELECT  cast(c1 as binary(10))    FROM my_binary_t01;
SELECT convert(c1, binary(10))    FROM my_binary_t01;
SELECT         c1::binary(10)     FROM my_binary_t01;

-- EXPECT: syntax error
SELECT  cast(c1 as varbinary(10)) FROM my_binary_t01;
SELECT convert(c1, varbinary(10)) FROM my_binary_t01;
SELECT         c1::varbinary(10)  FROM my_binary_t01;

-- EXPECT: syntax error
SELECT  cast(c1 as blob)          FROM my_binary_t01;
SELECT convert(c1, blob)          FROM my_binary_t01;
SELECT         c1::blob           FROM my_binary_t01;

-- EXPECT: syntax error
SELECT  cast(c1 as tinyblob)      FROM my_binary_t01;
SELECT convert(c1, tinyblob)      FROM my_binary_t01;
SELECT         c1::tinyblob       FROM my_binary_t01;

-- EXPECT: syntax error
SELECT  cast(c1 as mediumblob)    FROM my_binary_t01;
SELECT convert(c1, mediumblob)    FROM my_binary_t01;
SELECT         c1::mediumblob     FROM my_binary_t01;

-- EXPECT: syntax error
SELECT  cast(c1 as longblob)      FROM my_binary_t01;
SELECT convert(c1, longblob)      FROM my_binary_t01;
SELECT         c1::longblob       FROM my_binary_t01;

--------------------------------------------------------------------------------
-- use cast "mysql varbinary" to "mysql other binary"

-- EXPECT: ok
SELECT  cast(c1 as binary)        FROM my_varbinary_t02;
SELECT convert(c1, binary)        FROM my_varbinary_t02;
SELECT         c1::binary         FROM my_varbinary_t02;

-- EXPECT: ok, but warning
SELECT  cast(c1 as binary(0))     FROM my_varbinary_t02;
SELECT convert(c1, binary(0))     FROM my_varbinary_t02;
SELECT         c1::binary(0)      FROM my_varbinary_t02;

-- EXPECT: ok, but warning
SELECT  cast(c1 as binary(1))     FROM my_varbinary_t02;
SELECT convert(c1, binary(1))     FROM my_varbinary_t02;
SELECT         c1::binary(1)      FROM my_varbinary_t02;

-- EXPECT: ok
SELECT  cast(c1 as binary(10))    FROM my_varbinary_t02;
SELECT convert(c1, binary(10))    FROM my_varbinary_t02;
SELECT         c1::binary(10)     FROM my_varbinary_t02;

-- EXPECT: syntax error
-- TODO[TD003]: ok
SELECT  cast(c1 as varbinary(10)) FROM my_varbinary_t02;
SELECT convert(c1, varbinary(10)) FROM my_varbinary_t02;
SELECT         c1::varbinary(10)  FROM my_varbinary_t02;

-- EXPECT: syntax error
SELECT  cast(c1 as blob)          FROM my_varbinary_t02;
SELECT convert(c1, blob)          FROM my_varbinary_t02;
SELECT         c1::blob           FROM my_varbinary_t02;

-- EXPECT: syntax error
SELECT  cast(c1 as tinyblob)      FROM my_varbinary_t02;
SELECT convert(c1, tinyblob)      FROM my_varbinary_t02;
SELECT         c1::tinyblob       FROM my_varbinary_t02;

-- EXPECT: syntax error
SELECT  cast(c1 as mediumblob)    FROM my_varbinary_t02;
SELECT convert(c1, mediumblob)    FROM my_varbinary_t02;
SELECT         c1::mediumblob     FROM my_varbinary_t02;

-- EXPECT: syntax error
SELECT  cast(c1 as longblob)      FROM my_varbinary_t02;
SELECT convert(c1, longblob)      FROM my_varbinary_t02;
SELECT         c1::longblob       FROM my_varbinary_t02;

--------------------------------------------------------------------------------
-- use cast "mysql blob" to "mysql other binary"

-- EXPECT: ok
SELECT  cast(c1 as binary)        FROM my_blob_t03;
SELECT convert(c1, binary)        FROM my_blob_t03;
SELECT         c1::binary         FROM my_blob_t03;

-- EXPECT: ok, but warning
SELECT  cast(c1 as binary(0))     FROM my_blob_t03;
SELECT convert(c1, binary(0))     FROM my_blob_t03;
SELECT         c1::binary(0)      FROM my_blob_t03;

-- EXPECT: ok, but warning
SELECT  cast(c1 as binary(1))     FROM my_blob_t03;
SELECT convert(c1, binary(1))     FROM my_blob_t03;
SELECT         c1::binary(1)      FROM my_blob_t03;

-- EXPECT: ok
SELECT  cast(c1 as binary(10))    FROM my_blob_t03;
SELECT convert(c1, binary(10))    FROM my_blob_t03;
SELECT         c1::binary(10)     FROM my_blob_t03;

-- EXPECT: syntax error
SELECT  cast(c1 as varbinary(10)) FROM my_blob_t03;
SELECT convert(c1, varbinary(10)) FROM my_blob_t03;
SELECT         c1::varbinary(10)  FROM my_blob_t03;

-- EXPECT: syntax error
-- TODO[TD003]: ok
SELECT  cast(c1 as blob)          FROM my_blob_t03;
SELECT convert(c1, blob)          FROM my_blob_t03;
SELECT         c1::blob           FROM my_blob_t03;

-- EXPECT: syntax error
SELECT  cast(c1 as tinyblob)      FROM my_blob_t03;
SELECT convert(c1, tinyblob)      FROM my_blob_t03;
SELECT         c1::tinyblob       FROM my_blob_t03;

-- EXPECT: syntax error
SELECT  cast(c1 as mediumblob)    FROM my_blob_t03;
SELECT convert(c1, mediumblob)    FROM my_blob_t03;
SELECT         c1::mediumblob     FROM my_blob_t03;

-- EXPECT: syntax error
SELECT  cast(c1 as longblob)      FROM my_blob_t03;
SELECT convert(c1, longblob)      FROM my_blob_t03;
SELECT         c1::longblob       FROM my_blob_t03;

--------------------------------------------------------------------------------
-- use cast "mysql tinyblob" to "mysql other binary"

-- EXPECT: ok
SELECT  cast(c1 as binary)        FROM my_tinyblob_t04;
SELECT convert(c1, binary)        FROM my_tinyblob_t04;
SELECT         c1::binary         FROM my_tinyblob_t04;

-- EXPECT: ok, but warning
SELECT  cast(c1 as binary(0))     FROM my_tinyblob_t04;
SELECT convert(c1, binary(0))     FROM my_tinyblob_t04;
SELECT         c1::binary(0)      FROM my_tinyblob_t04;

-- EXPECT: ok, but warning
SELECT  cast(c1 as binary(1))     FROM my_tinyblob_t04;
SELECT convert(c1, binary(1))     FROM my_tinyblob_t04;
SELECT         c1::binary(1)      FROM my_tinyblob_t04;

-- EXPECT: ok
SELECT  cast(c1 as binary(10))    FROM my_tinyblob_t04;
SELECT convert(c1, binary(10))    FROM my_tinyblob_t04;
SELECT         c1::binary(10)     FROM my_tinyblob_t04;

-- EXPECT: syntax error
SELECT  cast(c1 as varbinary(10)) FROM my_tinyblob_t04;
SELECT convert(c1, varbinary(10)) FROM my_tinyblob_t04;
SELECT         c1::varbinary(10)  FROM my_tinyblob_t04;

-- EXPECT: syntax error
SELECT  cast(c1 as blob)          FROM my_tinyblob_t04;
SELECT convert(c1, blob)          FROM my_tinyblob_t04;
SELECT         c1::blob           FROM my_tinyblob_t04;

-- EXPECT: syntax error
-- TODO[TD003]: ok
SELECT  cast(c1 as tinyblob)      FROM my_tinyblob_t04;
SELECT convert(c1, tinyblob)      FROM my_tinyblob_t04;
SELECT         c1::tinyblob       FROM my_tinyblob_t04;

-- EXPECT: syntax error
SELECT  cast(c1 as mediumblob)    FROM my_tinyblob_t04;
SELECT convert(c1, mediumblob)    FROM my_tinyblob_t04;
SELECT         c1::mediumblob     FROM my_tinyblob_t04;

-- EXPECT: syntax error
SELECT  cast(c1 as longblob)      FROM my_tinyblob_t04;
SELECT convert(c1, longblob)      FROM my_tinyblob_t04;
SELECT         c1::longblob       FROM my_tinyblob_t04;

--------------------------------------------------------------------------------
-- use cast "mysql mediumblob" to "mysql other binary"

-- EXPECT: ok
SELECT  cast(c1 as binary)        FROM my_mediumblob_t05;
SELECT convert(c1, binary)        FROM my_mediumblob_t05;
SELECT         c1::binary         FROM my_mediumblob_t05;

-- EXPECT: ok, but warning
SELECT  cast(c1 as binary(0))     FROM my_mediumblob_t05;
SELECT convert(c1, binary(0))     FROM my_mediumblob_t05;
SELECT         c1::binary(0)      FROM my_mediumblob_t05;

-- EXPECT: ok, but warning
SELECT  cast(c1 as binary(1))     FROM my_mediumblob_t05;
SELECT convert(c1, binary(1))     FROM my_mediumblob_t05;
SELECT         c1::binary(1)      FROM my_mediumblob_t05;

-- EXPECT: ok
SELECT  cast(c1 as binary(10))    FROM my_mediumblob_t05;
SELECT convert(c1, binary(10))    FROM my_mediumblob_t05;
SELECT         c1::binary(10)     FROM my_mediumblob_t05;

-- EXPECT: syntax error
SELECT  cast(c1 as varbinary(10)) FROM my_mediumblob_t05;
SELECT convert(c1, varbinary(10)) FROM my_mediumblob_t05;
SELECT         c1::varbinary(10)  FROM my_mediumblob_t05;

-- EXPECT: syntax error
SELECT  cast(c1 as blob)          FROM my_mediumblob_t05;
SELECT convert(c1, blob)          FROM my_mediumblob_t05;
SELECT         c1::blob           FROM my_mediumblob_t05;

-- EXPECT: syntax error
SELECT  cast(c1 as tinyblob)      FROM my_mediumblob_t05;
SELECT convert(c1, tinyblob)      FROM my_mediumblob_t05;
SELECT         c1::tinyblob       FROM my_mediumblob_t05;

-- EXPECT: syntax error
-- TODO[TD003]: ok
SELECT  cast(c1 as mediumblob)    FROM my_mediumblob_t05;
SELECT convert(c1, mediumblob)    FROM my_mediumblob_t05;
SELECT         c1::mediumblob     FROM my_mediumblob_t05;

-- EXPECT: syntax error
SELECT  cast(c1 as longblob)      FROM my_mediumblob_t05;
SELECT convert(c1, longblob)      FROM my_mediumblob_t05;
SELECT         c1::longblob       FROM my_mediumblob_t05;

--------------------------------------------------------------------------------
-- use cast "mysql longblob" to "mysql other binary"

-- EXPECT: ok
SELECT  cast(c1 as binary)        FROM my_longblob_t06;
SELECT convert(c1, binary)        FROM my_longblob_t06;
SELECT         c1::binary         FROM my_longblob_t06;

-- EXPECT: ok, but warning
SELECT  cast(c1 as binary(0))     FROM my_longblob_t06;
SELECT convert(c1, binary(0))     FROM my_longblob_t06;
SELECT         c1::binary(0)      FROM my_longblob_t06;

-- EXPECT: ok, but warning
SELECT  cast(c1 as binary(1))     FROM my_longblob_t06;
SELECT convert(c1, binary(1))     FROM my_longblob_t06;
SELECT         c1::binary(1)      FROM my_longblob_t06;

-- EXPECT: ok
SELECT  cast(c1 as binary(10))    FROM my_longblob_t06;
SELECT convert(c1, binary(10))    FROM my_longblob_t06;
SELECT         c1::binary(10)     FROM my_longblob_t06;

-- EXPECT: syntax error
SELECT  cast(c1 as varbinary(10)) FROM my_longblob_t06;
SELECT convert(c1, varbinary(10)) FROM my_longblob_t06;
SELECT         c1::varbinary(10)  FROM my_longblob_t06;

-- EXPECT: syntax error
SELECT  cast(c1 as blob)          FROM my_longblob_t06;
SELECT convert(c1, blob)          FROM my_longblob_t06;
SELECT         c1::blob           FROM my_longblob_t06;

-- EXPECT: syntax error
SELECT  cast(c1 as tinyblob)      FROM my_longblob_t06;
SELECT convert(c1, tinyblob)      FROM my_longblob_t06;
SELECT         c1::tinyblob       FROM my_longblob_t06;

-- EXPECT: syntax error
SELECT  cast(c1 as mediumblob)    FROM my_longblob_t06;
SELECT convert(c1, mediumblob)    FROM my_longblob_t06;
SELECT         c1::mediumblob     FROM my_longblob_t06;

-- EXPECT: syntax error
-- TODO[TD003]: ok
SELECT  cast(c1 as longblob)      FROM my_longblob_t06;
SELECT convert(c1, longblob)      FROM my_longblob_t06;
SELECT         c1::longblob       FROM my_longblob_t06;

--------------------------------------------------------------------------------
-- use cast "mysql binary" to "mysql other type"

-- EXPECT: ok, {1234, abcd, 1q2w, q1w2}
-- TODO[TD004]: ok, but get diff value
SELECT  cast(c1 as char)          FROM my_binary_t07;
SELECT convert(c1, char)          FROM my_binary_t07;
SELECT         c1::char           FROM my_binary_t07;

-- EXPECT: ok , but warning
-- TODO[TD004]: error
SELECT  cast(c1 as date)          FROM my_binary_t07;
SELECT convert(c1, date)          FROM my_binary_t07;
SELECT         c1::date           FROM my_binary_t07;

-- EXPECT: ok , but warning
-- TODO[TD004]: error
SELECT  cast(c1 as time)          FROM my_binary_t07;
SELECT convert(c1, time)          FROM my_binary_t07;
SELECT         c1::time           FROM my_binary_t07;

-- EXPECT: ok , but warning
-- TODO[TD004]: error
SELECT  cast(c1 as datetime)      FROM my_binary_t07;
SELECT convert(c1, datetime)      FROM my_binary_t07;
SELECT         c1::datetime       FROM my_binary_t07;

-- EXPECT: ok , but warning
-- TODO[TD004]: error
SELECT  cast(c1 as decimal(4, 0)) FROM my_binary_t07;
SELECT convert(c1, decimal(4, 0)) FROM my_binary_t07;
SELECT         c1::decimal(4, 0)  FROM my_binary_t07;

-- EXPECT: ok
-- TODO[TD004]: error
SELECT  cast(c1 as json)          FROM my_binary_t07;
SELECT convert(c1, json)          FROM my_binary_t07;
SELECT         c1::json           FROM my_binary_t07;

-- EXPECT: ok, {1234, abcd, 1q2w, q1w2}
-- TODO[TD004]: ok, but get diff value
SELECT  cast(c1 as nchar)         FROM my_binary_t07;
SELECT convert(c1, nchar)         FROM my_binary_t07;
SELECT         c1::nchar          FROM my_binary_t07;

-- EXPECT: ok , but warning
-- TODO[TD004]: error
SELECT  cast(c1 as signed)        FROM my_binary_t07;
SELECT convert(c1, signed)        FROM my_binary_t07;
SELECT         c1::signed         FROM my_binary_t07;

-- EXPECT: ok , but warning
-- TODO[TD004]: error
SELECT  cast(c1 as unsigned)      FROM my_binary_t07;
SELECT convert(c1, unsigned)      FROM my_binary_t07;
SELECT         c1::unsigned       FROM my_binary_t07;

--------------------------------------------------------------------------------
-- use cast "mysql varbinary" to "mysql other type"

-- EXPECT: ok, {1234, abcd, 1q2w, q1w2}
-- TODO[TD004]: ok, but get diff value
SELECT  cast(c1 as char)          FROM my_varbinary_t08;
SELECT convert(c1, char)          FROM my_varbinary_t08;
SELECT         c1::char           FROM my_varbinary_t08;

-- TODO[TD013]: EXPECT: get {NULL, NULL, NULL, NULL}, and warning
SELECT  cast(c1 as date)          FROM my_varbinary_t08;
SELECT convert(c1, date)          FROM my_varbinary_t08;
SELECT         c1::date           FROM my_varbinary_t08;

-- EXPECT: get {00:12:34, 00:00:00, 00:00:01, 00:00:00}, and warning
SELECT  cast(c1 as time)          FROM my_varbinary_t08;
SELECT convert(c1, time)          FROM my_varbinary_t08;
SELECT         c1::time           FROM my_varbinary_t08;

-- TODO[TD013]: EXPECT: get {NULL, NULL, NULL, NULL}, and warning
SELECT  cast(c1 as datetime)      FROM my_varbinary_t08;
SELECT convert(c1, datetime)      FROM my_varbinary_t08;
SELECT         c1::datetime       FROM my_varbinary_t08;

-- EXPECT: ok , but warning
-- TODO[TD004]: error
SELECT  cast(c1 as decimal(4, 0)) FROM my_varbinary_t08;
SELECT convert(c1, decimal(4, 0)) FROM my_varbinary_t08;
SELECT         c1::decimal(4, 0)  FROM my_varbinary_t08;

-- EXPECT: ok
-- TODO[TD004]: error
SELECT  cast(c1 as json)          FROM my_varbinary_t08;
SELECT convert(c1, json)          FROM my_varbinary_t08;
SELECT         c1::json           FROM my_varbinary_t08;

-- EXPECT: ok, {1234, abcd, 1q2w, q1w2}
-- TODO[TD004]: ok, but get diff value
SELECT  cast(c1 as nchar)         FROM my_varbinary_t08;
SELECT convert(c1, nchar)         FROM my_varbinary_t08;
SELECT         c1::nchar          FROM my_varbinary_t08;

-- EXPECT: ok , but warning
-- TODO[TD004]: error
SELECT  cast(c1 as signed)        FROM my_varbinary_t08;
SELECT convert(c1, signed)        FROM my_varbinary_t08;
SELECT         c1::signed         FROM my_varbinary_t08;

-- EXPECT: ok , but warning
-- TODO[TD004]: error
SELECT  cast(c1 as unsigned)      FROM my_varbinary_t08;
SELECT convert(c1, unsigned)      FROM my_varbinary_t08;
SELECT         c1::unsigned       FROM my_varbinary_t08;

--------------------------------------------------------------------------------
-- use cast "mysql blob" to "mysql other type"

-- EXPECT: ok, {1234, abcd, 1q2w, q1w2}
-- TODO[TD004]: ok, but get diff value
SELECT  cast(c1 as char)          FROM my_blob_t09;
SELECT convert(c1, char)          FROM my_blob_t09;
SELECT         c1::char           FROM my_blob_t09;

-- EXPECT: ok , but warning
-- TODO[TD004]: error
SELECT  cast(c1 as date)          FROM my_blob_t09;
SELECT convert(c1, date)          FROM my_blob_t09;
SELECT         c1::date           FROM my_blob_t09;

-- EXPECT: ok , but warning
-- TODO[TD004]: error
SELECT  cast(c1 as time)          FROM my_blob_t09;
SELECT convert(c1, time)          FROM my_blob_t09;
SELECT         c1::time           FROM my_blob_t09;

-- EXPECT: ok , but warning
-- TODO[TD004]: error
SELECT  cast(c1 as datetime)      FROM my_blob_t09;
SELECT convert(c1, datetime)      FROM my_blob_t09;
SELECT         c1::datetime       FROM my_blob_t09;

-- EXPECT: ok , but warning
-- TODO[TD004]: error
SELECT  cast(c1 as decimal(4, 0)) FROM my_blob_t09;
SELECT convert(c1, decimal(4, 0)) FROM my_blob_t09;
SELECT         c1::decimal(4, 0)  FROM my_blob_t09;

-- EXPECT: ok
-- TODO[TD004]: error
SELECT  cast(c1 as json)          FROM my_blob_t09;
SELECT convert(c1, json)          FROM my_blob_t09;
SELECT         c1::json           FROM my_blob_t09;

-- EXPECT: ok, {1234, abcd, 1q2w, q1w2}
-- TODO[TD004]: ok, but get diff value
SELECT  cast(c1 as nchar)         FROM my_blob_t09;
SELECT convert(c1, nchar)         FROM my_blob_t09;
SELECT         c1::nchar          FROM my_blob_t09;

-- EXPECT: ok , but warning
-- TODO[TD004]: error
SELECT  cast(c1 as signed)        FROM my_blob_t09;
SELECT convert(c1, signed)        FROM my_blob_t09;
SELECT         c1::signed         FROM my_blob_t09;

-- EXPECT: ok , but warning
-- TODO[TD004]: error
SELECT  cast(c1 as unsigned)      FROM my_blob_t09;
SELECT convert(c1, unsigned)      FROM my_blob_t09;
SELECT         c1::unsigned       FROM my_blob_t09;

--------------------------------------------------------------------------------
-- use cast "mysql tinyblob" to "mysql other type"

-- EXPECT: ok, {1234, abcd, 1q2w, q1w2}
-- TODO[TD004]: ok, but get diff value
SELECT  cast(c1 as char)          FROM my_tinyblob_t10;
SELECT convert(c1, char)          FROM my_tinyblob_t10;
SELECT         c1::char           FROM my_tinyblob_t10;

-- EXPECT: ok , but warning
-- TODO[TD004]: error
SELECT  cast(c1 as date)          FROM my_tinyblob_t10;
SELECT convert(c1, date)          FROM my_tinyblob_t10;
SELECT         c1::date           FROM my_tinyblob_t10;

-- EXPECT: ok , but warning
-- TODO[TD004]: error
SELECT cast(c1 as time)          FROM my_tinyblob_t10;
SELECT convert(c1, time)          FROM my_tinyblob_t10;
SELECT         c1::time           FROM my_tinyblob_t10;

-- EXPECT: ok , but warning
-- TODO[TD004]: error
SELECT  cast(c1 as datetime)      FROM my_tinyblob_t10;
SELECT convert(c1, datetime)      FROM my_tinyblob_t10;
SELECT         c1::datetime       FROM my_tinyblob_t10;

-- EXPECT: ok , but warning
-- TODO[TD004]: error
SELECT  cast(c1 as decimal(4, 0)) FROM my_tinyblob_t10;
SELECT convert(c1, decimal(4, 0)) FROM my_tinyblob_t10;
SELECT         c1::decimal(4, 0)  FROM my_tinyblob_t10;

-- EXPECT: ok
-- TODO[TD004]: error
SELECT  cast(c1 as json)          FROM my_tinyblob_t10;
SELECT convert(c1, json)          FROM my_tinyblob_t10;
SELECT         c1::json           FROM my_tinyblob_t10;

-- EXPECT: ok, {1234, abcd, 1q2w, q1w2}
-- TODO[TD004]: ok, but get diff value
SELECT  cast(c1 as nchar)         FROM my_tinyblob_t10;
SELECT convert(c1, nchar)         FROM my_tinyblob_t10;
SELECT         c1::nchar          FROM my_tinyblob_t10;

-- EXPECT: ok , but warning
-- TODO[TD004]: error
SELECT  cast(c1 as signed)        FROM my_tinyblob_t10;
SELECT convert(c1, signed)        FROM my_tinyblob_t10;
SELECT         c1::signed         FROM my_tinyblob_t10;

-- EXPECT: ok , but warning
-- TODO[TD004]: error
SELECT  cast(c1 as unsigned)      FROM my_tinyblob_t10;
SELECT convert(c1, unsigned)      FROM my_tinyblob_t10;
SELECT         c1::unsigned       FROM my_tinyblob_t10;

--------------------------------------------------------------------------------
-- use cast "mysql mediumblob" to "mysql other type"

-- EXPECT: ok, {1234, abcd, 1q2w, q1w2}
-- TODO[TD004]: ok, but get diff value
SELECT  cast(c1 as char)          FROM my_mediumblob_t11;
SELECT convert(c1, char)          FROM my_mediumblob_t11;
SELECT         c1::char           FROM my_mediumblob_t11;

-- EXPECT: ok , but warning
-- TODO[TD004]: error
SELECT  cast(c1 as date)          FROM my_mediumblob_t11;
SELECT convert(c1, date)          FROM my_mediumblob_t11;
SELECT         c1::date           FROM my_mediumblob_t11;

-- EXPECT: ok , but warning
-- TODO[TD004]: error
SELECT  cast(c1 as time)          FROM my_mediumblob_t11;
SELECT convert(c1, time)          FROM my_mediumblob_t11;
SELECT         c1::time           FROM my_mediumblob_t11;

-- EXPECT: ok , but warning
-- TODO[TD004]: error
SELECT  cast(c1 as datetime)      FROM my_mediumblob_t11;
SELECT convert(c1, datetime)      FROM my_mediumblob_t11;
SELECT         c1::datetime       FROM my_mediumblob_t11;

-- EXPECT: ok , but warning
-- TODO[TD004]: error
SELECT  cast(c1 as decimal(4, 0)) FROM my_mediumblob_t11;
SELECT convert(c1, decimal(4, 0)) FROM my_mediumblob_t11;
SELECT         c1::decimal(4, 0)  FROM my_mediumblob_t11;

-- EXPECT: ok
-- TODO[TD004]: error
SELECT  cast(c1 as json)          FROM my_mediumblob_t11;
SELECT convert(c1, json)          FROM my_mediumblob_t11;
SELECT         c1::json           FROM my_mediumblob_t11;

-- EXPECT: ok, {1234, abcd, 1q2w, q1w2}
-- TODO[TD004]: ok, but get diff value
SELECT  cast(c1 as nchar)         FROM my_mediumblob_t11;
SELECT convert(c1, nchar)         FROM my_mediumblob_t11;
SELECT         c1::nchar          FROM my_mediumblob_t11;

-- EXPECT: ok , but warning
-- TODO[TD004]: error
SELECT  cast(c1 as signed)        FROM my_mediumblob_t11;
SELECT convert(c1, signed)        FROM my_mediumblob_t11;
SELECT         c1::signed         FROM my_mediumblob_t11;

-- EXPECT: ok , but warning
-- TODO[TD004]: error
SELECT  cast(c1 as unsigned)      FROM my_mediumblob_t11;
SELECT convert(c1, unsigned)      FROM my_mediumblob_t11;
SELECT         c1::unsigned       FROM my_mediumblob_t11;

--------------------------------------------------------------------------------
-- use cast "mysql longblob" to "mysql other type"

-- EXPECT: ok, {1234, abcd, 1q2w, q1w2}
-- TODO[TD004]: ok, but get diff value
SELECT  cast(c1 as char)          FROM my_longblob_t12;
SELECT convert(c1, char)          FROM my_longblob_t12;
SELECT         c1::char           FROM my_longblob_t12;

-- EXPECT: ok , but warning
-- TODO[TD004]: error
SELECT  cast(c1 as date)          FROM my_longblob_t12;
SELECT convert(c1, date)          FROM my_longblob_t12;
SELECT         c1::date           FROM my_longblob_t12;

-- EXPECT: ok , but warning
-- TODO[TD004]: error
SELECT  cast(c1 as time)          FROM my_longblob_t12;
SELECT convert(c1, time)          FROM my_longblob_t12;
SELECT         c1::time           FROM my_longblob_t12;

-- EXPECT: ok , but warning
-- TODO[TD004]: error
SELECT  cast(c1 as datetime)      FROM my_longblob_t12;
SELECT convert(c1, datetime)      FROM my_longblob_t12;
SELECT         c1::datetime       FROM my_longblob_t12;

-- EXPECT: ok , but warning
-- TODO[TD004]: error
SELECT  cast(c1 as decimal(4, 0)) FROM my_longblob_t12;
SELECT convert(c1, decimal(4, 0)) FROM my_longblob_t12;
SELECT         c1::decimal(4, 0)  FROM my_longblob_t12;

-- EXPECT: ok
-- TODO[TD004]: error
SELECT  cast(c1 as json)          FROM my_longblob_t12;
SELECT convert(c1, json)          FROM my_longblob_t12;
SELECT         c1::json           FROM my_longblob_t12;

-- EXPECT: ok, {1234, abcd, 1q2w, q1w2}
-- TODO[TD004]: ok, but get diff value
SELECT  cast(c1 as nchar)         FROM my_longblob_t12;
SELECT convert(c1, nchar)         FROM my_longblob_t12;
SELECT         c1::nchar          FROM my_longblob_t12;

-- EXPECT: ok , but warning
-- TODO[TD004]: error
SELECT  cast(c1 as signed)        FROM my_longblob_t12;
SELECT convert(c1, signed)        FROM my_longblob_t12;
SELECT         c1::signed         FROM my_longblob_t12;

-- EXPECT: ok , but warning
-- TODO[TD004]: error
SELECT  cast(c1 as unsigned)      FROM my_longblob_t12;
SELECT convert(c1, unsigned)      FROM my_longblob_t12;
SELECT         c1::unsigned       FROM my_longblob_t12;

--------------------------------------------------------------------------------
-- use cast "mysql binary" to "pg bytea"

-- EXPECT: ok
SELECT  cast(c1 as bytea) FROM my_binary_t13;
SELECT convert(c1, bytea) FROM my_binary_t13;
SELECT         c1::bytea  FROM my_binary_t13;

-- EXPECT: ok
SELECT  cast(c2 as bytea) FROM my_binary_t13;
SELECT convert(c2, bytea) FROM my_binary_t13;
SELECT         c2::bytea  FROM my_binary_t13;

-- EXPECT: ok
SELECT  cast(c3 as bytea) FROM my_binary_t13;
SELECT convert(c3, bytea) FROM my_binary_t13;
SELECT         c3::bytea  FROM my_binary_t13;

-- EXPECT: ok
SELECT  cast(c4 as bytea) FROM my_binary_t13;
SELECT convert(c4, bytea) FROM my_binary_t13;
SELECT         c4::bytea  FROM my_binary_t13;

-- EXPECT: ok
SELECT  cast(c5 as bytea) FROM my_binary_t13;
SELECT convert(c5, bytea) FROM my_binary_t13;
SELECT         c5::bytea  FROM my_binary_t13;

-- EXPECT: ok
SELECT  cast(c6 as bytea) FROM my_binary_t13;
SELECT convert(c6, bytea) FROM my_binary_t13;
SELECT         c6::bytea  FROM my_binary_t13;

--------------------------------------------------------------------------------
-- use cast "pg bytea" to "mysql binary"

-- EXPECT: ok
SELECT  cast(c1 as binary(10)) FROM pg_bytea_t14;
SELECT convert(c1, binary(10)) FROM pg_bytea_t14;
SELECT         c1::binary(10)  FROM pg_bytea_t14;

-- EXPECT: ok
SELECT  cast(c1 as varbinary(10)) FROM pg_bytea_t14;
SELECT convert(c1, varbinary(10)) FROM pg_bytea_t14;
SELECT         c1::varbinary(10)  FROM pg_bytea_t14;

-- EXPECT: ok
SELECT  cast(c1 as blob) FROM pg_bytea_t14;
SELECT convert(c1, blob) FROM pg_bytea_t14;
SELECT         c1::blob  FROM pg_bytea_t14;

-- EXPECT: ok
SELECT  cast(c1 as tinyblob) FROM pg_bytea_t14;
SELECT convert(c1, tinyblob) FROM pg_bytea_t14;
SELECT         c1::tinyblob  FROM pg_bytea_t14;

-- EXPECT: ok
SELECT  cast(c1 as mediumblob) FROM pg_bytea_t14;
SELECT convert(c1, mediumblob) FROM pg_bytea_t14;
SELECT         c1::mediumblob  FROM pg_bytea_t14;

-- EXPECT: ok
SELECT  cast(c1 as longblob) FROM pg_bytea_t14;
SELECT convert(c1, longblob) FROM pg_bytea_t14;
SELECT         c1::longblob  FROM pg_bytea_t14;

set mysql.mysql_convert = false;
set mysql.cast_to_char = off;