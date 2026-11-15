--
-- kek_dek_cross_schema.sql — KEK/DEK 跨 schema 绑定回归测试（Bug 9256）
--
-- 背景：
--   DEK 的目录记录里存的是 KEK 的 kek_id(OID)，而 KEK 可以创建在任意 schema。
--   修复前运行时取 KEK 走的是"DEK 记录里的裸 KEK 名 + search_path"解析，KEK 不在
--   search_path 上时永远找不到：
--       INSERT INTO <表> ... 报
--       Failed to get KEK by name 'kek_x' for DEK: dek_y
--       TECC SCHEDULER ERROR: Failed to get DEK key for encryption
--   修复后：优先按 KEK OID 解析（与 schema/search_path 无关），仅当 OID 无效时
--   才退回按名解析。
--
-- 覆盖：
--   1. 场景二：KEK 在 sch_cs1、DEK+表在 public（热缓存）
--   2. 场景二冷缓存：＼c 重连后按 OID 从数据库加载 KEK
--   3. 场景一：KEK+DEK 都在 sch_cs1、表在 public
--   4. 两个 schema 存在同名 KEK 时不得绑错密钥：DEK 绑 sch_cs1 的那把，
--      search_path 指向 sch_cs2（裸名会解析到 sch_cs2 的那把）
--   5. DET 等值查询（解密路径同样要跨 schema 取 KEK）
-- ============================================================

-- ============================================================
-- 0. 清理 + 准备
-- ============================================================
DROP TABLE IF EXISTS t_cs_pub CASCADE;
DROP TABLE IF EXISTS t_cs_sch CASCADE;
DROP TABLE IF EXISTS t_cs_same CASCADE;
DROP DEK IF EXISTS dek_cs_pub CASCADE;
DROP DEK IF EXISTS sch_cs1.dek_cs_sch CASCADE;
DROP DEK IF EXISTS dek_cs_same CASCADE;
DROP KEK IF EXISTS sch_cs1.kek_cs1 CASCADE;
DROP KEK IF EXISTS sch_cs2.kek_cs1 CASCADE;
DROP SCHEMA IF EXISTS sch_cs1 CASCADE;
DROP SCHEMA IF EXISTS sch_cs2 CASCADE;

CREATE SCHEMA sch_cs1;
CREATE SCHEMA sch_cs2;

-- ============================================================
-- 1. 场景二：KEK 在 sch_cs1，DEK 与表在 public
--    KEK 由 CREATE DEK 顺带进入本连接缓存，此处走热缓存路径
-- ============================================================
CREATE KEK sch_cs1.kek_cs1 WITH (
    key_store = 'localkms',
    key_algo  = 'sm4',
    key_path  = '/tmp/kek_cs1.key'
);

CREATE DEK dek_cs_pub WITH VALUES(KEK = sch_cs1.kek_cs1, KEY_ALGO = sm4);

CREATE TABLE t_cs_pub (
    id     int,
    name   text_enc  ENCRYPTED WITH (DEK = dek_cs_pub, SUPPORT_ALGO = 'DET'),
    salary int4_enc  ENCRYPTED WITH (DEK = dek_cs_pub, SUPPORT_ALGO = 'DET')
);

INSERT INTO t_cs_pub VALUES (1, 'highgo', 5000);
SELECT 'case1 KEK in sch_cs1, DEK+table in public', id, name, salary FROM t_cs_pub ORDER BY id;

-- ============================================================
-- 2. 冷缓存：重连清空连接级 KEK 缓存，KEK 只能按 OID 从目录加载
--    修复前此处的 INSERT 直接失败（load_kek_from_database_by_oid 为桩实现）
-- ============================================================
\c postgres postgres

INSERT INTO t_cs_pub VALUES (2, 'cold', 6000);
SELECT 'case2 cold cache (by OID load)', id, name, salary FROM t_cs_pub ORDER BY id;
SELECT 'case2 DET equality', id, name FROM t_cs_pub WHERE name = 'highgo';

-- ============================================================
-- 3. 场景一：KEK 与 DEK 都在 sch_cs1，表在 public
-- ============================================================
CREATE DEK sch_cs1.dek_cs_sch WITH VALUES(KEK = sch_cs1.kek_cs1, KEY_ALGO = sm4);

CREATE TABLE t_cs_sch (
    id   int,
    name text_enc ENCRYPTED WITH (DEK = sch_cs1.dek_cs_sch, SUPPORT_ALGO = 'DET')
);

INSERT INTO t_cs_sch VALUES (1, 'scenario1');
SELECT 'case3 KEK+DEK in sch_cs1', id, name FROM t_cs_sch ORDER BY id;

\c postgres postgres

SELECT 'case3 cold cache', id, name FROM t_cs_sch ORDER BY id;
SELECT 'case3 DET equality', id, name FROM t_cs_sch WHERE name = 'scenario1';

-- ============================================================
-- 4. 同名 KEK 分处两个 schema：DEK 绑的是 sch_cs1 那把，
--    而 search_path 指向 sch_cs2。若按裸名 + search_path 解析，
--    会取到 sch_cs2 的另一把密钥 → 解密必然失败；按 OID 解析才正确。
-- ============================================================
CREATE KEK sch_cs2.kek_cs1 WITH (
    key_store = 'localkms',
    key_algo  = 'sm4',
    key_path  = '/tmp/kek_cs1_other.key'
);

CREATE DEK dek_cs_same WITH VALUES(KEK = sch_cs1.kek_cs1, KEY_ALGO = sm4);

CREATE TABLE t_cs_same (
    id   int,
    name text_enc ENCRYPTED WITH (DEK = dek_cs_same, SUPPORT_ALGO = 'DET')
);

SET search_path = sch_cs2, public;

INSERT INTO t_cs_same VALUES (1, 'right-key');
SELECT 'case4 same KEK name in two schemas', id, name FROM t_cs_same ORDER BY id;

\c postgres postgres
SET search_path = sch_cs2, public;

SELECT 'case4 cold cache', id, name FROM t_cs_same ORDER BY id;
SELECT 'case4 DET equality', id, name FROM t_cs_same WHERE name = 'right-key';

RESET search_path;

-- ============================================================
-- 99. 清理
-- ============================================================
DROP TABLE t_cs_pub CASCADE;
DROP TABLE t_cs_sch CASCADE;
DROP TABLE t_cs_same CASCADE;
DROP DEK dek_cs_pub;
DROP DEK sch_cs1.dek_cs_sch;
DROP DEK dek_cs_same;
DROP KEK sch_cs1.kek_cs1 CASCADE;
DROP KEK sch_cs2.kek_cs1 CASCADE;
DROP SCHEMA sch_cs1;
DROP SCHEMA sch_cs2;
