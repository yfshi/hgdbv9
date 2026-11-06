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

INSERT INTO test_ud_t1 VALUES (1, 'Alice', 50000, 'IT');
INSERT INTO test_ud_t1 VALUES (2, 'Bob', 60000, 'HR');
INSERT INTO test_ud_t1 VALUES (3, 'Charlie', 70000, 'IT');
INSERT INTO test_ud_t1 VALUES (4, 'Diana', 80000, 'HR');

-- U1: Basic UPDATE SET encrypted
UPDATE test_ud_t1 SET salary = 55000 WHERE id = 1;
SELECT '1a_update_basic', id, name, salary FROM test_ud_t1 WHERE id = 1;

-- U2: WHERE encrypted column comparison
SELECT '1b_where_enc', id, name FROM test_ud_t1 WHERE salary > 55000;

-- U3: UPDATE target alias
UPDATE test_ud_t1 t SET salary = 52000 WHERE t.id = 1;
UPDATE test_ud_t1 AS t SET salary = 53000 WHERE t.id = 2;
SELECT '1c_update_alias', id, salary FROM test_ud_t1 WHERE id IN (1, 2) ORDER BY id;

-- Restore
UPDATE test_ud_t1 SET salary = 50000 WHERE id = 1;
UPDATE test_ud_t1 SET salary = 60000 WHERE id = 2;

-- U4: UPDATE FROM multi-table
CREATE TABLE test_ud_t2 (id INT4, ref_name VARCHAR(100), bonus INT4, dept VARCHAR(50));
INSERT INTO test_ud_t2 VALUES (1, 'ref_a', 1000, 'IT'), (2, 'ref_b', 2000, 'HR'), (3, 'ref_c', 3000, 'IT');

UPDATE test_ud_t1 t1 SET salary = 58000
FROM test_ud_t2 t2 WHERE t1.id = t2.id AND t2.bonus = 1000;
SELECT '1d_update_from', id, salary FROM test_ud_t1 WHERE id = 1;

-- U5: UPDATE FROM with encrypted WHERE
CREATE TABLE test_ud_t3 (id INT4, secret TEXT_ENC ENCRYPTED WITH (DEK = dek_ud, SUPPORT_ALGO = 'DET'), score INT4);
INSERT INTO test_ud_t3 VALUES (1, 'secret_x', 90), (2, 'secret_y', 80);

UPDATE test_ud_t1 t1 SET salary = 62000
FROM test_ud_t3 t3 WHERE t1.id = t3.id AND t3.secret = 'secret_x';
SELECT '1e_update_from_enc', t1.id, t1.salary FROM test_ud_t1 t1
JOIN test_ud_t3 t3 ON t1.id = t3.id WHERE t3.secret = 'secret_x';

-- U6: UPDATE FROM multi-encrypted JOIN
UPDATE test_ud_t1 SET salary = 65000
FROM test_ud_t2 t2 JOIN test_ud_t3 t3 ON t2.id = t3.id
WHERE test_ud_t1.id = t3.id AND t3.secret = 'secret_y';
SELECT '1f_update_join_enc', t1.id, t1.salary FROM test_ud_t1 t1
JOIN test_ud_t3 t3 ON t1.id = t3.id WHERE t3.secret = 'secret_y';

-- Restore
UPDATE test_ud_t1 SET salary = 50000 WHERE id = 1;
UPDATE test_ud_t1 SET salary = 60000 WHERE id = 2;

-- U7: UPDATE RETURNING
UPDATE test_ud_t1 SET salary = 72000 WHERE id = 1 RETURNING id, salary;
UPDATE test_ud_t1 SET salary = 50000 WHERE id = 1;

-- D1: DELETE WHERE encrypted
SELECT '1g_where_enc_ctrl', id, name FROM test_ud_t1 WHERE salary > 70000;

-- D2: DELETE alias
DELETE FROM test_ud_t1 t WHERE t.id = 99;
SELECT '1h_delete_alias', count(*) FROM test_ud_t1;

-- D3: DELETE USING
INSERT INTO test_ud_t1 VALUES (99, 'temp', 99999, 'XX');
DELETE FROM test_ud_t1 t1 USING test_ud_t2 t2 WHERE t1.id = t2.id AND t1.id = 99;
SELECT '1i_delete_using', count(*) FROM test_ud_t1 WHERE id = 99;

-- D4: DELETE USING with encrypted WHERE
INSERT INTO test_ud_t1 VALUES (98, 'temp2', 88888, 'YY');
DELETE FROM test_ud_t1 t1 USING test_ud_t3 t3
WHERE t1.id = t3.id AND t3.secret = 'secret_x' AND t1.id = 98;
SELECT '1j_delete_using_enc', count(*) FROM test_ud_t1 WHERE id = 98;

-- D5: DELETE USING JOIN
INSERT INTO test_ud_t1 VALUES (97, 'temp3', 77777, 'ZZ');
DELETE FROM test_ud_t1 t1 USING test_ud_t2 t2 JOIN test_ud_t3 t3 ON t2.id = t3.id
WHERE t1.id = t3.id AND t3.secret = 'secret_y' AND t1.id = 97;
SELECT '1k_delete_using_join', count(*) FROM test_ud_t1 WHERE id = 97;

DELETE FROM test_ud_t1 WHERE id > 10;
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
INSERT INTO test_ud_conflict VALUES (1, 'initial', 10000);
INSERT INTO test_ud_conflict VALUES (1, 'updated', 20000)
ON CONFLICT (id) DO UPDATE SET salary = 20000, name = 'updated';
SELECT '2a_on_conflict', id, name, salary FROM test_ud_conflict WHERE id = 1;

-- C1b: Multi-column on conflict
INSERT INTO test_ud_conflict VALUES (1, 'again', 99999)
ON CONFLICT (id) DO UPDATE SET salary = 30000, name = 'updated_again';
SELECT '2b_on_conflict_multi', id, name, salary FROM test_ud_conflict WHERE id = 1;

-- C2: ON CONFLICT DO NOTHING
INSERT INTO test_ud_conflict VALUES (1, 'ignored', 30000) ON CONFLICT (id) DO NOTHING;
SELECT '2c_on_conflict_nothing', id, name, salary FROM test_ud_conflict WHERE id = 1;

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

INSERT INTO test_ud_row VALUES (1, 'Alice', 50000, 'IT');

UPDATE test_ud_row SET (salary, name) = ROW(72000, 'multi_col_test') WHERE id = 1;
SELECT '3a_row_update', id, salary, name FROM test_ud_row WHERE id = 1;

DROP TABLE IF EXISTS test_ud_row CASCADE;

-- ============================================================
-- Cleanup
-- ============================================================
SELECT '=== Cleanup ===';
DROP KEK IF EXISTS kek_ud CASCADE;
