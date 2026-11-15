-- ============================================================
-- pbe.sql — PBE参数化查询测试
-- 覆盖 test_pbe_encrypted, test_pbe_simple,
--   test_prepare_encrypted, test_prepare_execute_deallocate
-- ============================================================

-- ============================================================
-- 0. Setup
-- ============================================================
DROP KEK IF EXISTS kek_pbe CASCADE;

CREATE KEK kek_pbe WITH (key_store = 'localkms', key_path = '/tmp/kek_pbe.key');

CREATE DEK dek_pbe WITH VALUES(KEK = kek_pbe);

-- ============================================================
-- 1. PREPARE/EXECUTE on plain table (basic syntax)
-- ============================================================
SELECT '=== 1 PREPARE/EXECUTE Basic ===';

DROP TABLE IF EXISTS test_prep_t1 CASCADE;

CREATE TABLE test_prep_t1 (id INT4, name VARCHAR(100), salary INT4, dept VARCHAR(50));

-- @param int 1
-- @param String Alice
-- @param int 50000
-- @param String IT
-- @param int 2
-- @param String Bob
-- @param int 60000
-- @param String HR
INSERT INTO test_prep_t1 VALUES (?, ?, ?, ?), (?, ?, ?, ?);

-- P1: PREPARE SELECT basic
PREPARE p1 AS SELECT * FROM test_prep_t1 WHERE salary > 45000;

EXECUTE p1;

DEALLOCATE p1;

-- P2: PREPARE with type list
PREPARE p2 (int4, varchar) AS SELECT * FROM test_prep_t1 WHERE salary > $1 AND name = $2;

EXECUTE p2(40000, 'Bob');

DEALLOCATE p2;

-- P3: PREPARE INSERT
PREPARE p3 (int4, varchar, int4, varchar) AS INSERT INTO test_prep_t1 VALUES ($1, $2, $3, $4);

EXECUTE p3(3, 'Charlie', 70000, 'IT');

-- @param int 3
SELECT 'p3_check', id, name FROM test_prep_t1 WHERE id = ?;

DEALLOCATE p3;

-- P4: PREPARE UPDATE
PREPARE p4 (int4, int4) AS UPDATE test_prep_t1 SET salary = $1 WHERE id = $2;

EXECUTE p4(55000, 1);

-- @param int 1
SELECT 'p4_check', id, salary FROM test_prep_t1 WHERE id = ?;

DEALLOCATE p4;

-- P5: PREPARE DELETE
PREPARE p5 (int4) AS DELETE FROM test_prep_t1 WHERE id = $1;

DEALLOCATE p5;

-- E1: EXECUTE INSERT with params
PREPARE e1 (int4, varchar, int4, varchar) AS INSERT INTO test_prep_t1 VALUES ($1, $2, $3, $4);

EXECUTE e1(4, 'Diana', 80000, 'HR');

-- @param int 4
SELECT 'e1_check', id, name FROM test_prep_t1 WHERE id = ?;

DEALLOCATE e1;

-- E2: EXECUTE no params
PREPARE e2 AS SELECT count(*) FROM test_prep_t1 WHERE salary > 55000;

EXECUTE e2;

DEALLOCATE e2;

-- DEALLOCATE forms
PREPARE d1 AS SELECT * FROM test_prep_t1 WHERE id = 1;

DEALLOCATE d1;

PREPARE d2 AS SELECT * FROM test_prep_t1 WHERE id = 2;

DEALLOCATE PREPARE d2;

PREPARE d3a AS SELECT * FROM test_prep_t1 WHERE id = 1;

PREPARE d3b AS SELECT * FROM test_prep_t1 WHERE id = 2;

DEALLOCATE ALL;

PREPARE d4a AS SELECT * FROM test_prep_t1 WHERE id = 1;

DEALLOCATE PREPARE ALL;

-- EXPLAIN with PREPARE
EXPLAIN SELECT * FROM test_prep_t1 WHERE salary > 50000;

PREPARE x2 AS SELECT * FROM test_prep_t1 WHERE id = 1;

EXPLAIN EXECUTE x2;

DEALLOCATE x2;

DROP TABLE IF EXISTS test_prep_t1 CASCADE;

-- ============================================================
-- 2. PREPARE/EXECUTE with encrypted columns (test_prepare_encrypted)
-- ============================================================
SELECT '=== 2 PREPARE + Encrypted ===';

CREATE TABLE test_pe_t1 (
    id INT4, name VARCHAR(100),
    salary INT4_ENC ENCRYPTED WITH (DEK = dek_pbe, SUPPORT_ALGO = 'ope'),
    dept VARCHAR(50)
);

-- PREPARE INSERT with encrypted constant
PREPARE c1 AS INSERT INTO test_pe_t1 VALUES (1, 'Alice', 50000, 'IT');

EXECUTE c1;

-- @param int 1
SELECT '2a_insert_const', id, name FROM test_pe_t1 WHERE id = ?;

DEALLOCATE c1;

-- PREPARE SELECT with WHERE encrypted constant
PREPARE c2 AS SELECT id, name FROM test_pe_t1 WHERE salary > 45000;

EXECUTE c2;

DEALLOCATE c2;

-- PREPARE UPDATE encrypted column
PREPARE c3 AS UPDATE test_pe_t1 SET salary = 55000 WHERE id = 1;

EXECUTE c3;

-- @param int 1
SELECT '2b_update_const', id, salary FROM test_pe_t1 WHERE id = ?;

DEALLOCATE c3;

-- PREPARE DELETE with encrypted WHERE
PREPARE c4 AS DELETE FROM test_pe_t1 WHERE salary > 50000;

EXECUTE c4;

SELECT '2c_delete_cnt', count(*) FROM test_pe_t1;

-- Multi-encrypted table
CREATE TABLE test_pe_t2 (
    id INT4,
    name VARCHAR_ENC(100) ENCRYPTED WITH (DEK = dek_pbe),
    salary INT4_ENC ENCRYPTED WITH (DEK = dek_pbe),
    dept VARCHAR(50)
);

PREPARE c7 AS INSERT INTO test_pe_t2 VALUES (1, 'Carol', 70000, 'IT');

EXECUTE c7;

-- @param int 1
SELECT '2d_multi_enc', id, name FROM test_pe_t2 WHERE id = ?;

DEALLOCATE c7;

-- INSERT with NULL for encrypted column
PREPARE c8 AS INSERT INTO test_pe_t2 VALUES (2, 'Dave', NULL, 'HR');

EXECUTE c8;

-- @param int 2
SELECT '2e_enc_null', id, name FROM test_pe_t2 WHERE id = ?;

DEALLOCATE c8;

-- UPDATE varchar_enc column
PREPARE c9 AS UPDATE test_pe_t2 SET name = 'Eve' WHERE id = 1;

EXECUTE c9;

-- @param int 1
SELECT '2f_update_varchar_enc', id, name FROM test_pe_t2 WHERE id = ?;

DEALLOCATE c9;

-- DEALLOCATE with encrypted
PREPARE d1 AS SELECT id, name FROM test_pe_t1 WHERE salary > 50000;

DEALLOCATE d1;

PREPARE d2a AS INSERT INTO test_pe_t1 VALUES (99, 'TestA', 99999, 'QA');

PREPARE d2b AS INSERT INTO test_pe_t1 VALUES (98, 'TestB', 88888, 'QA');

DEALLOCATE ALL;

DROP TABLE IF EXISTS test_pe_t1 CASCADE;

DROP TABLE IF EXISTS test_pe_t2 CASCADE;

-- ============================================================
-- 3. PBE full (parameterized) — test_pbe_encrypted
-- ============================================================
SELECT '=== 3 PBE Parameterized ===';

CREATE TABLE test_pbe_t1 (
    id INT4, name VARCHAR(100),
    salary INT4_ENC ENCRYPTED WITH (DEK = dek_pbe, SUPPORT_ALGO = 'OPE'),
    dept VARCHAR(50)
);

-- INSERT with 1 encrypted param
PREPARE a1 (int4, varchar, int4_enc, varchar) AS
    INSERT INTO test_pbe_t1 VALUES ($1, $2, $3, $4);

EXECUTE a1(1, 'Alice', 50000, 'IT');

-- @param int 1
SELECT '3a_insert_param', id, name, salary FROM test_pbe_t1 WHERE id = ?;

DEALLOCATE a1;

-- SELECT WHERE = encrypted param
PREPARE b1 (int4_enc) AS SELECT id, name, salary, dept FROM test_pbe_t1 WHERE salary = $1;

EXECUTE b1(50000);

DEALLOCATE b1;

-- SELECT WHERE > encrypted param
PREPARE b2 (int4_enc) AS SELECT id, name, salary, dept FROM test_pbe_t1 WHERE salary > $1;

EXECUTE b2(45000);

DEALLOCATE b2;

-- SELECT WHERE BETWEEN encrypted params
PREPARE b4 (int4_enc, int4_enc) AS SELECT id, name, salary, dept FROM test_pbe_t1 WHERE salary BETWEEN $1 AND $2;

EXECUTE b4(40000, 65000);

DEALLOCATE b4;

-- UPDATE SET encrypted param
PREPARE c1 (int4_enc, int4) AS UPDATE test_pbe_t1 SET salary = $1 WHERE id = $2;

EXECUTE c1(55000, 1);

-- @param int 1
SELECT '3b_update_param', id, salary FROM test_pbe_t1 WHERE id = ?;

DEALLOCATE c1;

-- DELETE WHERE encrypted param
-- @param int 3
-- @param String Charlie
-- @param int 70000
-- @param String Temp
INSERT INTO test_pbe_t1 VALUES (?, ?, ?, ?);

PREPARE d1 (int4_enc) AS DELETE FROM test_pbe_t1 WHERE salary = $1;

EXECUTE d1(70000);

-- @param int 3
SELECT '3c_delete_param_cnt', count(*) FROM test_pbe_t1 WHERE id = ?;

DEALLOCATE d1;

-- Multi-encrypted params
CREATE TABLE test_pbe_t2 (
    id INT4,
    name VARCHAR_ENC(100) ENCRYPTED WITH (DEK = dek_pbe, SUPPORT_ALGO = 'DET'),
    age INT4_ENC ENCRYPTED WITH (DEK = dek_pbe, SUPPORT_ALGO = 'OPE'),
    score FLOAT8_ENC ENCRYPTED WITH (DEK = dek_pbe, SUPPORT_ALGO = 'OPE'),
    bio TEXT_ENC ENCRYPTED WITH (DEK = dek_pbe, SUPPORT_ALGO = 'DET,SEARCH'),
    dept VARCHAR(50)
);

PREPARE e1 (int4, varchar_enc, int4_enc, float8_enc, text_enc, varchar) AS
    INSERT INTO test_pbe_t2 VALUES ($1, $2, $3, $4, $5, $6);

EXECUTE e1(1, 'Alice', 30, 95.5, 'Alice bio', 'IT');

EXECUTE e1(2, 'Bob', 25, 88.0, 'Bob bio', 'HR');

SELECT '3d_multi_enc_insert', id, name, age, score FROM test_pbe_t2 ORDER BY id;

DEALLOCATE e1;

-- SELECT multi-encrypted WHERE
PREPARE e2 (int4_enc, float8_enc) AS SELECT id, name, age, score FROM test_pbe_t2 WHERE age > $1 AND score > $2;

EXECUTE e2(28, 90.0);

DEALLOCATE e2;

-- SELECT WHERE varchar_enc
PREPARE e3 (varchar_enc) AS SELECT id, name, age FROM test_pbe_t2 WHERE name = $1;

EXECUTE e3('Alice');

DEALLOCATE e3;

-- SELECT WHERE text_enc LIKE
PREPARE e4 (text_enc) AS SELECT id, name, bio FROM test_pbe_t2 WHERE bio LIKE $1;

EXECUTE e4('%Alice%');

DEALLOCATE e4;

-- UPDATE multi-encrypted
PREPARE e5 (int4_enc, float8_enc, int4) AS UPDATE test_pbe_t2 SET age = $1, score = $2 WHERE id = $3;

EXECUTE e5(31, 96.0, 1);

-- @param int 1
SELECT '3e_multi_update', id, age, score FROM test_pbe_t2 WHERE id = ?;

DEALLOCATE e5;

-- NULL params
PREPARE f1 (int4, int4_enc) AS INSERT INTO test_pbe_t1 VALUES ($1, 'NullTest', $2, 'QA');

EXECUTE f1(10, NULL);

-- @param int 10
SELECT '3f_null_param', id, name, salary FROM test_pbe_t1 WHERE id = ?;

DEALLOCATE f1;

-- Mixed plain + encrypted params
PREPARE h1 (int4, varchar, int4_enc, varchar) AS INSERT INTO test_pbe_t1 VALUES ($1, $2, $3, $4);

EXECUTE h1(20, 'Mixed', 75000, 'Dev');

-- @param int 20
SELECT '3g_mixed_params', id, name, salary FROM test_pbe_t1 WHERE id = ?;

DEALLOCATE h1;

-- Mixed WHERE condition
PREPARE h2 (varchar, int4_enc) AS SELECT id, name, salary FROM test_pbe_t1 WHERE dept = $1 AND salary > $2;

EXECUTE h2('IT', 50000);

DEALLOCATE h2;

-- BPCHAR_ENC test
CREATE TABLE test_pbe_t3 (id INT4, code BPCHAR_ENC(8) ENCRYPTED WITH (DEK = dek_pbe, SUPPORT_ALGO = 'DET'), label VARCHAR(50));

PREPARE i1 (int4, bpchar_enc, varchar) AS INSERT INTO test_pbe_t3 VALUES ($1, $2, $3);

EXECUTE i1(1, 'CODE001', 'Test code 1');

SELECT '3h_bpchar_enc', id, code, label FROM test_pbe_t3 ORDER BY id;

DEALLOCATE i1;

PREPARE i2 (bpchar_enc) AS SELECT id, code, label FROM test_pbe_t3 WHERE code = $1;

EXECUTE i2('CODE001');

DEALLOCATE i2;

DROP TABLE IF EXISTS test_pbe_t1 CASCADE;

DROP TABLE IF EXISTS test_pbe_t2 CASCADE;

DROP TABLE IF EXISTS test_pbe_t3 CASCADE;

-- ============================================================
-- 4. Simple PBE (test_pbe_simple)
-- ============================================================
SELECT '=== 4 Simple PBE ===';

DROP TABLE IF EXISTS t1 CASCADE;

CREATE TABLE t1 (id INT4, name VARCHAR(100), salary INT4_ENC ENCRYPTED WITH (DEK = dek_pbe, SUPPORT_ALGO = 'DET'));

PREPARE s1 (int4, varchar, int4_enc) AS INSERT INTO t1 VALUES ($1, $2, $3);

EXECUTE s1(1, 'Alice', 50000);

EXECUTE s1(2, 'Bob', 60000);

DEALLOCATE s1;

PREPARE s2 (int4_enc) AS SELECT id, name, salary FROM t1 WHERE salary = $1;

EXECUTE s2(50000);

DEALLOCATE s2;

DROP TABLE t1 CASCADE;

-- ============================================================
-- Cleanup
-- ============================================================
SELECT '=== Cleanup ===';

DROP KEK IF EXISTS kek_pbe CASCADE;
