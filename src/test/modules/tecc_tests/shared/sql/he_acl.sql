-- ============================================================
-- he_acl.sql — 非对称 KEK/DEK 的 ACL 权限回归测试
-- ReqID:TECC4.0
-- 覆盖：GRANT/REVOKE USAGE ON KEK(sm2) / DEK(he-add/he-mul)
--       CREATE DEK 需 KEK USAGE；建表引用 HE DEK 需 DEK USAGE
--       角色权限继承、owner 默认权限、异常权限组合
-- ============================================================

-- 0. Setup（幂等清理 + 生成 sm2 KEK + 建用户/角色）
DROP TABLE IF EXISTS he_acl_owner CASCADE;
DROP TABLE IF EXISTS he_acl_u1 CASCADE;
DROP TABLE IF EXISTS he_acl_u1b CASCADE;
DROP TABLE IF EXISTS he_acl_u1c CASCADE;
DROP KEK IF EXISTS kek_acl CASCADE;
DO $$
BEGIN
   IF EXISTS (SELECT 1 FROM pg_roles WHERE rolname = 'u1') THEN DROP OWNED BY u1; END IF;
   IF EXISTS (SELECT 1 FROM pg_roles WHERE rolname = 'u2') THEN DROP OWNED BY u2; END IF;
   IF EXISTS (SELECT 1 FROM pg_roles WHERE rolname = 'r1') THEN DROP OWNED BY r1; END IF;
END $$;
DROP ROLE IF EXISTS u1, u2, r1;

\! rm -f "$TECC_KEK_PATH/kek_acl.priv" "$TECC_KEK_PATH/kek_acl.pub" 2>/dev/null
\! tecc_ktool generate --key-name kek_acl --key-algo sm2 --key-path "$TECC_KEK_PATH" >/dev/null 2>&1

CREATE USER u1 WITH PASSWORD 'test123';
CREATE USER u2 WITH PASSWORD 'test123';
CREATE ROLE r1;

-- owner 建 sm2 KEK
CREATE KEK kek_acl WITH (
    key_store = 'localkms',
    key_algo = 'sm2',
    key_path = '%KEKPATH%/kek_acl.priv'
);

-- owner 建 HE DEK（he-add + he-mul）
CREATE DEK dek_acl_add WITH VALUES(KEK = kek_acl, KEY_ALGO = 'he-add');
CREATE DEK dek_acl_mul WITH VALUES(KEK = kek_acl, KEY_ALGO = 'he-mul');

-- 用户建对象需要 schema 权限
GRANT USAGE ON SCHEMA public TO u1, u2;
GRANT CREATE ON SCHEMA public TO u1, u2;

-- ============================================================
-- 1. KEK USAGE 权限 → CREATE DEK
-- ============================================================
SELECT '=== 1 KEK USAGE -> CREATE DEK ===';

-- 1a. u1 无 USAGE ON KEK → CREATE DEK he-add 报 permission denied
SET ROLE u1;
CREATE DEK dek_u1_add WITH VALUES(KEK = kek_acl, KEY_ALGO = 'he-add');
RESET ROLE;

-- 1b. 授权 USAGE ON KEK 后 → CREATE DEK he-add 成功
GRANT USAGE ON KEK kek_acl TO u1;
SET ROLE u1;
CREATE DEK dek_u1_add WITH VALUES(KEK = kek_acl, KEY_ALGO = 'he-add');
RESET ROLE;

-- 1c. u1 CREATE DEK he-mul → 成功
SET ROLE u1;
CREATE DEK dek_u1_mul WITH VALUES(KEK = kek_acl, KEY_ALGO = 'he-mul');
RESET ROLE;

-- 1d. u1 CREATE 对称 DEK（绑非对称 KEK）→ 成功（对称 DEK 也允许绑非对称 KEK）
SET ROLE u1;
CREATE DEK dek_u1_sym WITH VALUES(KEK = kek_acl);
RESET ROLE;

-- 1e. REVOKE USAGE ON KEK 后 → CREATE DEK 报 permission denied
REVOKE USAGE ON KEK kek_acl FROM u1;
SET ROLE u1;
CREATE DEK dek_u1_add2 WITH VALUES(KEK = kek_acl, KEY_ALGO = 'he-add');
RESET ROLE;

-- ============================================================
-- 2. DEK USAGE 权限 → 建表
-- ============================================================
SELECT '=== 2 DEK USAGE -> CREATE TABLE ===';

-- 2a. u1 无 USAGE ON DEK → 建表引用 HE DEK 报 permission denied
SET ROLE u1;
CREATE TABLE he_acl_u1 (
    id SERIAL,
    c INT4_ENC ENCRYPTED WITH (HE_ADD_DEK = 'dek_acl_add', SUPPORT_ALGO = 'he-add')
);
RESET ROLE;

-- 2b. 授权 USAGE ON DEK 后 → 建表成功
GRANT USAGE ON DEK dek_acl_add TO u1;
SET ROLE u1;
CREATE TABLE he_acl_u1 (
    id SERIAL,
    c INT4_ENC ENCRYPTED WITH (HE_ADD_DEK = 'dek_acl_add', SUPPORT_ALGO = 'he-add')
);
RESET ROLE;

-- 2c. u1 INSERT 加密（有 DEK USAGE + KEK 公钥）→ 成功（加密仅需公钥）
SET ROLE u1;
INSERT INTO he_acl_u1 (c) VALUES (10), (20), (30);
RESET ROLE;

-- 2d. REVOKE USAGE ON DEK 后 → 建表报 permission denied
REVOKE USAGE ON DEK dek_acl_add FROM u1;
SET ROLE u1;
CREATE TABLE he_acl_u1b (
    id SERIAL,
    c INT4_ENC ENCRYPTED WITH (HE_ADD_DEK = 'dek_acl_add', SUPPORT_ALGO = 'he-add')
);
RESET ROLE;

-- ============================================================
-- 3. 角色权限继承
-- ============================================================
SELECT '=== 3 role inheritance ===';

-- 3a. 授权 KEK USAGE 给角色 r1，r1 授予 u2 → u2 继承后 CREATE DEK 成功
GRANT USAGE ON KEK kek_acl TO r1;
GRANT r1 TO u2;
SET ROLE u2;
CREATE DEK dek_u2_add WITH VALUES(KEK = kek_acl, KEY_ALGO = 'he-add');
RESET ROLE;

-- 3b. REVOKE r1 FROM u2 后 → u2 CREATE DEK 报 permission denied
REVOKE r1 FROM u2;
SET ROLE u2;
CREATE DEK dek_u2_add2 WITH VALUES(KEK = kek_acl, KEY_ALGO = 'he-add');
RESET ROLE;

-- ============================================================
-- 4. owner 自身（无需显式 GRANT，owner 有全部权限）
-- ============================================================
SELECT '=== 4 owner self ===';
CREATE TABLE he_acl_owner (
    id SERIAL,
    c INT4_ENC ENCRYPTED WITH (HE_ADD_DEK = 'dek_acl_add', SUPPORT_ALGO = 'he-add')
);
INSERT INTO he_acl_owner (c) VALUES (1), (2);
SELECT id, c FROM he_acl_owner ORDER BY id;

-- ============================================================
-- 5. 异常组合
-- ============================================================
SELECT '=== 5 error combos ===';

-- 5a. KEK 有权限 + DEK 无权限 → 建表仍报 permission denied（DEK USAGE 缺失）
--     （u1 当前无 KEK USAGE、无 DEK USAGE；先恢复 KEK USAGE 再建表）
GRANT USAGE ON KEK kek_acl TO u1;
SET ROLE u1;
CREATE TABLE he_acl_u1c (
    id SERIAL,
    c INT4_ENC ENCRYPTED WITH (HE_ADD_DEK = 'dek_acl_add', SUPPORT_ALGO = 'he-add')
);
RESET ROLE;

-- 5b. 无 schema CREATE 权限 → CREATE DEK 报 schema 权限错
REVOKE CREATE ON SCHEMA public FROM u1, PUBLIC;
SET ROLE u1;
CREATE DEK dek_u1_add3 WITH VALUES(KEK = kek_acl, KEY_ALGO = 'he-add');
RESET ROLE;
GRANT CREATE ON SCHEMA public TO u1, PUBLIC;

-- ============================================================
-- 6. Cleanup
-- ============================================================
DROP TABLE IF EXISTS he_acl_owner CASCADE;
DROP TABLE IF EXISTS he_acl_u1 CASCADE;
DROP TABLE IF EXISTS he_acl_u1b CASCADE;
DROP TABLE IF EXISTS he_acl_u1c CASCADE;
DROP KEK IF EXISTS kek_acl CASCADE;
DO $$
BEGIN
   IF EXISTS (SELECT 1 FROM pg_roles WHERE rolname = 'u1') THEN DROP OWNED BY u1; END IF;
   IF EXISTS (SELECT 1 FROM pg_roles WHERE rolname = 'u2') THEN DROP OWNED BY u2; END IF;
   IF EXISTS (SELECT 1 FROM pg_roles WHERE rolname = 'r1') THEN DROP OWNED BY r1; END IF;
END $$;
DROP ROLE IF EXISTS u1, u2, r1;

-- ============================================================
-- has_kek_privilege / has_dek_privilege 函数变体测试
-- (从 tecc_acl_queries / tecc_acl_edge_cases 迁移)
-- ============================================================

-- Setup
CREATE USER acl_u1;
CREATE USER acl_u2;
CREATE USER acl_u3;

CREATE KEK acl_test_kek WITH (
    KEY_STORE = 'localkms',
    KEY_PATH = '%KEKPATH%/acl_test.key'
);
CREATE DEK acl_test_dek WITH VALUES (KEK = 'acl_test_kek');

-- 1. Owner默认有USAGE
SELECT has_kek_privilege('acl_test_kek', 'USAGE') AS owner_kek;
SELECT has_dek_privilege('acl_test_dek', 'USAGE') AS owner_dek;

-- 2. 其他用户初始无权限
SELECT has_kek_privilege('acl_u1', 'acl_test_kek', 'USAGE') AS u1_init_kek;
SELECT has_dek_privilege('acl_u1', 'acl_test_dek', 'USAGE') AS u1_init_dek;

-- 3. GRANT后检查 - 6种函数变体
GRANT USAGE ON KEK acl_test_kek TO acl_u1;
GRANT USAGE ON DEK acl_test_dek TO acl_u1;

-- 变体1: username, keyname
SELECT has_kek_privilege('acl_u1', 'acl_test_kek', 'USAGE') AS v1_kek;
SELECT has_dek_privilege('acl_u1', 'acl_test_dek', 'USAGE') AS v1_dek;

-- 变体2: username, key OID
SELECT has_kek_privilege('acl_u1', oid, 'USAGE') FROM hg_kek WHERE kek_name = 'acl_test_kek';
SELECT has_dek_privilege('acl_u1', oid, 'USAGE') FROM hg_dek WHERE dek_name = 'acl_test_dek';

-- 变体3: user OID, keyname
SELECT has_kek_privilege(oid, 'acl_test_kek', 'USAGE') FROM pg_authid WHERE rolname = 'acl_u1';
SELECT has_dek_privilege(oid, 'acl_test_dek', 'USAGE') FROM pg_authid WHERE rolname = 'acl_u1';

-- 变体4: user OID, key OID
SELECT has_kek_privilege(u.oid, k.oid, 'USAGE')
FROM pg_authid u, hg_kek k WHERE u.rolname = 'acl_u1' AND k.kek_name = 'acl_test_kek';
SELECT has_dek_privilege(u.oid, d.oid, 'USAGE')
FROM pg_authid u, hg_dek d WHERE u.rolname = 'acl_u1' AND d.dek_name = 'acl_test_dek';

-- 变体5: keyname (当前用户)
SET SESSION AUTHORIZATION acl_u1;
SELECT has_kek_privilege('acl_test_kek', 'USAGE') AS cur_kek;
SELECT has_dek_privilege('acl_test_dek', 'USAGE') AS cur_dek;
RESET SESSION AUTHORIZATION;

-- 变体6: key OID (当前用户)
SET SESSION AUTHORIZATION acl_u1;
SELECT has_kek_privilege(oid, 'USAGE') FROM hg_kek WHERE kek_name = 'acl_test_kek';
SELECT has_dek_privilege(oid, 'USAGE') FROM hg_dek WHERE dek_name = 'acl_test_dek';
RESET SESSION AUTHORIZATION;

-- 4. WITH GRANT OPTION + 级联
GRANT USAGE ON KEK acl_test_kek TO acl_u2 WITH GRANT OPTION;
GRANT USAGE ON DEK acl_test_dek TO acl_u2 WITH GRANT OPTION;
SET SESSION AUTHORIZATION acl_u2;
GRANT USAGE ON KEK acl_test_kek TO acl_u3;
GRANT USAGE ON DEK acl_test_dek TO acl_u3;
RESET SESSION AUTHORIZATION;
SELECT has_kek_privilege('acl_u3', 'acl_test_kek', 'USAGE') AS u3_after_grant;
SELECT has_dek_privilege('acl_u3', 'acl_test_dek', 'USAGE') AS u3_after_grant;

-- 5. REVOKE
REVOKE USAGE ON KEK acl_test_kek FROM acl_u1;
REVOKE USAGE ON DEK acl_test_dek FROM acl_u1;
SELECT has_kek_privilege('acl_u1', 'acl_test_kek', 'USAGE') AS u1_after_revoke;
SELECT has_dek_privilege('acl_u1', 'acl_test_dek', 'USAGE') AS u1_after_revoke;

-- 6. 边界: NULL/空字符串
SELECT has_kek_privilege(NULL, 'USAGE') AS null_key;
SELECT has_dek_privilege(NULL, 'USAGE') AS null_dek;
SELECT has_kek_privilege('acl_test_kek', NULL) AS null_priv;
SELECT has_dek_privilege('acl_test_dek', NULL) AS null_priv;

-- 7. 边界: 无效参数
SELECT has_kek_privilege(0, 'USAGE') AS invalid_oid;
SELECT has_dek_privilege(0, 'USAGE') AS invalid_oid;
SELECT has_kek_privilege('acl_test_kek', 'INVALID') AS invalid_priv;
SELECT has_dek_privilege('acl_test_dek', 'INVALID') AS invalid_priv;

-- Cleanup
REVOKE ALL ON KEK acl_test_kek FROM acl_u2, acl_u3;
REVOKE ALL ON DEK acl_test_dek FROM acl_u2, acl_u3;
DROP DEK IF EXISTS acl_test_dek;
DROP KEK IF EXISTS acl_test_kek;
DROP USER IF EXISTS acl_u1;
DROP USER IF EXISTS acl_u2;
DROP USER IF EXISTS acl_u3;
