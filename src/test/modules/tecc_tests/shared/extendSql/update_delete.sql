-- ============================================================
-- update_delete.sql — UPDATE/DELETE/ON CONFLICT测试
-- 覆盖 test_update_delete_encrypted, test_tecc_20cases,
--   test_tecc_combination
-- ============================================================

-- ============================================================
-- 0. Setup
-- ============================================================
DROP KEK IF EXISTS kek_ud CASCADE;

CREATE KEK kek_ud WITH (key_store = 'localkms', key_path = '/tmp/kek_ud.key');

CREATE DEK dek_ud WITH VALUES(KEK = kek_ud);

-- ============================================================
-- 1. Basic UPDATE/DELETE with encrypted columns
-- ============================================================
SELECT '=== 1 Basic UPDATE/DELETE ===';

CREATE TABLE test_ud_t1 (
    id INT4, name VARCHAR(100),
    salary INT4_ENC ENCRYPTED WITH (DEK = dek_ud, SUPPORT_ALGO = 'ope'),
    dept VARCHAR(50)
);

-- @param int 1
-- @param String Alice
-- @param int 50000
-- @param String IT
INSERT INTO test_ud_t1 VALUES (?, ?, ?, ?);

-- @param int 2
-- @param String Bob
-- @param int 60000
-- @param String HR
INSERT INTO test_ud_t1 VALUES (?, ?, ?, ?);

-- @param int 3
-- @param String Charlie
-- @param int 70000
-- @param String IT
INSERT INTO test_ud_t1 VALUES (?, ?, ?, ?);

-- @param int 4
-- @param String Diana
-- @param int 80000
-- @param String HR
INSERT INTO test_ud_t1 VALUES (?, ?, ?, ?);

-- U1: Basic UPDATE SET encrypted
-- @param int 55000
-- @param int 1
UPDATE test_ud_t1 SET salary = ? WHERE id = ?;

-- @param int 1
SELECT '1a_update_basic', id, name, salary FROM test_ud_t1 WHERE id = ?;

-- U2: WHERE encrypted column comparison
-- @param int 55000
SELECT '1b_where_enc', id, name FROM test_ud_t1 WHERE salary > ?;

-- U3: UPDATE target alias
-- @param int 52000
-- @param int 1
UPDATE test_ud_t1 t SET salary = ? WHERE t.id = ?;

-- @param int 53000
-- @param int 2
UPDATE test_ud_t1 AS t SET salary = ? WHERE t.id = ?;

-- @param int 1
-- @param int 2
SELECT '1c_update_alias', id, salary FROM test_ud_t1 WHERE id IN (?, ?) ORDER BY id;

-- Restore
-- @param int 50000
-- @param int 1
UPDATE test_ud_t1 SET salary = ? WHERE id = ?;

-- @param int 60000
-- @param int 2
UPDATE test_ud_t1 SET salary = ? WHERE id = ?;

-- U4: UPDATE FROM multi-table
CREATE TABLE test_ud_t2 (id INT4, ref_name VARCHAR(100), bonus INT4, dept VARCHAR(50));

-- @param int 1
-- @param String ref_a
-- @param int 1000
-- @param String IT
-- @param int 2
-- @param String ref_b
-- @param int 2000
-- @param String HR
-- @param int 3
-- @param String ref_c
-- @param int 3000
-- @param String IT
INSERT INTO test_ud_t2 VALUES (?, ?, ?, ?), (?, ?, ?, ?), (?, ?, ?, ?);

-- @param int 58000
-- @param int 1000
UPDATE test_ud_t1 t1 SET salary = ?
FROM test_ud_t2 t2 WHERE t1.id = t2.id AND t2.bonus = ?;

-- @param int 1
SELECT '1d_update_from', id, salary FROM test_ud_t1 WHERE id = ?;

-- U5: UPDATE FROM with encrypted WHERE
CREATE TABLE test_ud_t3 (id INT4, secret TEXT_ENC ENCRYPTED WITH (DEK = dek_ud, SUPPORT_ALGO = 'DET'), score INT4);

-- @param int 1
-- @param String secret_x
-- @param int 90
-- @param int 2
-- @param String secret_y
-- @param int 80
INSERT INTO test_ud_t3 VALUES (?, ?, ?), (?, ?, ?);

-- @param int 62000
-- @param String secret_x
UPDATE test_ud_t1 t1 SET salary = ?
FROM test_ud_t3 t3 WHERE t1.id = t3.id AND t3.secret = ?;

-- @param String secret_x
SELECT '1e_update_from_enc', t1.id, t1.salary FROM test_ud_t1 t1
JOIN test_ud_t3 t3 ON t1.id = t3.id WHERE t3.secret = ?;

-- U6: UPDATE FROM multi-encrypted JOIN
-- @param int 65000
-- @param String secret_y
UPDATE test_ud_t1 SET salary = ?
FROM test_ud_t2 t2 JOIN test_ud_t3 t3 ON t2.id = t3.id
WHERE test_ud_t1.id = t3.id AND t3.secret = ?;

-- @param String secret_y
SELECT '1f_update_join_enc', t1.id, t1.salary FROM test_ud_t1 t1
JOIN test_ud_t3 t3 ON t1.id = t3.id WHERE t3.secret = ?;

-- Restore
-- @param int 50000
-- @param int 1
UPDATE test_ud_t1 SET salary = ? WHERE id = ?;

-- @param int 60000
-- @param int 2
UPDATE test_ud_t1 SET salary = ? WHERE id = ?;

-- U7: UPDATE RETURNING
-- @param int 72000
-- @param int 1
UPDATE test_ud_t1 SET salary = ? WHERE id = ? RETURNING id, salary;

-- @param int 50000
-- @param int 1
UPDATE test_ud_t1 SET salary = ? WHERE id = ?;

-- D1: DELETE WHERE encrypted
-- @param int 70000
SELECT '1g_where_enc_ctrl', id, name FROM test_ud_t1 WHERE salary > ?;

-- D2: DELETE alias
-- @param int 99
DELETE FROM test_ud_t1 t WHERE t.id = ?;

SELECT '1h_delete_alias', count(*) FROM test_ud_t1;

-- D3: DELETE USING
-- @param int 99
-- @param String temp
-- @param int 99999
-- @param String XX
INSERT INTO test_ud_t1 VALUES (?, ?, ?, ?);

-- @param int 99
DELETE FROM test_ud_t1 t1 USING test_ud_t2 t2 WHERE t1.id = t2.id AND t1.id = ?;

-- @param int 99
SELECT '1i_delete_using', count(*) FROM test_ud_t1 WHERE id = ?;

-- D4: DELETE USING with encrypted WHERE
-- @param int 98
-- @param String temp2
-- @param int 88888
-- @param String YY
INSERT INTO test_ud_t1 VALUES (?, ?, ?, ?);

-- @param String secret_x
-- @param int 98
DELETE FROM test_ud_t1 t1 USING test_ud_t3 t3
WHERE t1.id = t3.id AND t3.secret = ? AND t1.id = ?;

-- @param int 98
SELECT '1j_delete_using_enc', count(*) FROM test_ud_t1 WHERE id = ?;

-- D5: DELETE USING JOIN
-- @param int 97
-- @param String temp3
-- @param int 77777
-- @param String ZZ
INSERT INTO test_ud_t1 VALUES (?, ?, ?, ?);

-- @param String secret_y
-- @param int 97
DELETE FROM test_ud_t1 t1 USING test_ud_t2 t2 JOIN test_ud_t3 t3 ON t2.id = t3.id
WHERE t1.id = t3.id AND t3.secret = ? AND t1.id = ?;

-- @param int 97
SELECT '1k_delete_using_join', count(*) FROM test_ud_t1 WHERE id = ?;

-- @param int 10
DELETE FROM test_ud_t1 WHERE id > ?;

SELECT '1l_cleanup_extra', count(*) FROM test_ud_t1;

DROP TABLE IF EXISTS test_ud_t1 CASCADE;

DROP TABLE IF EXISTS test_ud_t2 CASCADE;

DROP TABLE IF EXISTS test_ud_t3 CASCADE;

-- ============================================================
-- 2. INSERT...ON CONFLICT (UPSERT)
-- ============================================================
SELECT '=== 2 INSERT...ON CONFLICT ===';

DROP TABLE IF EXISTS test_ud_conflict CASCADE;

CREATE TABLE test_ud_conflict (
    id INT4 PRIMARY KEY,
    name VARCHAR(100),
    salary INT4_ENC ENCRYPTED WITH (DEK = dek_ud, SUPPORT_ALGO = 'ope')
);

-- C1: INSERT...ON CONFLICT DO UPDATE SET encrypted
-- @param int 1
-- @param String initial
-- @param int 10000
INSERT INTO test_ud_conflict VALUES (?, ?, ?);

-- @param int 1
-- @param String updated
-- @param int 20000
-- @param int 20000
-- @param String updated
INSERT INTO test_ud_conflict VALUES (?, ?, ?)
ON CONFLICT (id) DO UPDATE SET salary = ?, name = ?;

-- @param int 1
SELECT '2a_on_conflict', id, name, salary FROM test_ud_conflict WHERE id = ?;

-- C1b: Multi-column on conflict
-- @param int 1
-- @param String again
-- @param int 99999
-- @param int 30000
-- @param String updated_again
INSERT INTO test_ud_conflict VALUES (?, ?, ?)
ON CONFLICT (id) DO UPDATE SET salary = ?, name = ?;

-- @param int 1
SELECT '2b_on_conflict_multi', id, name, salary FROM test_ud_conflict WHERE id = ?;

-- C2: ON CONFLICT DO NOTHING
-- @param int 1
-- @param String ignored
-- @param int 30000
INSERT INTO test_ud_conflict VALUES (?, ?, ?) ON CONFLICT (id) DO NOTHING;

-- @param int 1
SELECT '2c_on_conflict_nothing', id, name, salary FROM test_ud_conflict WHERE id = ?;

DROP TABLE IF EXISTS test_ud_conflict CASCADE;

-- ============================================================
-- 3. UPDATE multi-column SET with ROW()
-- ============================================================
SELECT '=== 3 UPDATE ROW() ===';

CREATE TABLE test_ud_row (
    id INT4, name VARCHAR(100),
    salary INT4_ENC ENCRYPTED WITH (DEK = dek_ud, SUPPORT_ALGO = 'ope'),
    dept VARCHAR(50)
);

-- @param int 1
-- @param String Alice
-- @param int 50000
-- @param String IT
INSERT INTO test_ud_row VALUES (?, ?, ?, ?);

-- @param int 72000
-- @param String multi_col_test
-- @param int 1
UPDATE test_ud_row SET (salary, name) = ROW(?, ?) WHERE id = ?;

-- @param int 1
SELECT '3a_row_update', id, salary, name FROM test_ud_row WHERE id = ?;

DROP TABLE IF EXISTS test_ud_row CASCADE;

-- ============================================================
-- Cleanup
-- ============================================================
SELECT '=== Cleanup ===';

DROP KEK IF EXISTS kek_ud CASCADE;
