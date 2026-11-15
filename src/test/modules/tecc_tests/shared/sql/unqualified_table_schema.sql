--
-- unqualified_table_schema.sql — 未限定表名按 search_path 服务器解析回归测试
--
-- 背景（Review 遗留说明 C 修复）：
--   修复前：前端对未限定表名（RangeVar.schemaname == NULL）硬编码查 public，
--   与 PG 的 RangeVarGetRelid 语义（临时表 → pg_catalog → search_path 顺序）
--   不一致：表在非 public schema 时漏检（SELECT 返回密文 / INSERT 类型报错）。
--   修复后：未限定表名经 tecc_stmt_resolve_table_schema 按优先级解析：
--   显式限定名 → 会话级临时表清单 → 服务器解析（data fetcher 池连接按
--   search_path 查 pg_class，table_schema_cache 缓存，search_path 变更失效）
--   → current_schema → public。
--
-- 判定手段：
--   a) 表只在 sp1 存在（public 无同名表）：修复前查 public 无加密列 →
--      INSERT 明文透传 → 服务器类型不匹配报错；修复后解析 sp1 → 正常加密。
--   b) 同名遮蔽（sp1.t3 普通 + public.t3 加密，search_path = sp1, public）：
--      PG 解析 sp1.t3（普通表）；修复前误用 public.t3 加密改写 → 类型报错；
--      修复后解析 sp1 → 不改写 → 正常。
--   c) 表在 search_path 第二项（sp1, public 而表在 public）：不回归。
-- ============================================================

-- ============================================================
-- 0. Setup：kek_pub/dek_pub 建在 public；kek_sp1/dek_sp1 建在 sp1
-- ============================================================
DROP SCHEMA IF EXISTS sp1 CASCADE;
DROP KEK IF EXISTS kek_pub CASCADE;
DROP KEK IF EXISTS kek_sp1 CASCADE;
DROP DEK IF EXISTS dek_pub CASCADE;
DROP DEK IF EXISTS dek_sp1 CASCADE;

CREATE KEK kek_pub WITH (key_store = 'localkms', key_path = '/tmp/kek_pub.key');
CREATE SCHEMA sp1;
SET search_path = sp1;
CREATE KEK kek_sp1 WITH (key_store = 'localkms', key_path = '/tmp/kek_sp1.key');
CREATE DEK dek_sp1 WITH VALUES(KEK = kek_sp1);
RESET search_path;
CREATE DEK dek_pub WITH VALUES(KEK = kek_pub);

-- ============================================================
-- 1. 表只在 sp1（未限定访问应解析到 sp1）
--    修复前：查 public.t_only 无加密列 → INSERT 明文透传 → 类型报错；
--    修复后：解析 sp1 → 正常加密/解密。
-- ============================================================
SET search_path = sp1;
CREATE TABLE t_only (id int, v int4_enc ENCRYPTED WITH (DEK = 'dek_sp1', support_algo = 'det'));
INSERT INTO t_only VALUES (1, 100);
SELECT id, v FROM t_only ORDER BY id;

-- ============================================================
-- 2. 表在 search_path 第二项（sp1, public 而表在 public）——不回归
-- ============================================================
RESET search_path;
CREATE TABLE t2 (id int, v int4_enc ENCRYPTED WITH (DEK = 'dek_pub', support_algo = 'det'));
SET search_path = sp1, public;
INSERT INTO t2 VALUES (2, 200);
SELECT id, v FROM t2 ORDER BY id;

-- ============================================================
-- 3. 同名遮蔽：sp1.t3 普通表 + public.t3 加密表
--    PG 解析 sp1.t3（普通表）；修复前误用 public.t3 加密改写 → 报错；
--    修复后解析 sp1 → 明文操作成功。
-- ============================================================
CREATE TABLE t3 (id int, v int);   -- 建在 sp1（search_path 首项）
RESET search_path;
CREATE TABLE t3 (id int, v int4_enc ENCRYPTED WITH (DEK = 'dek_pub', support_algo = 'det'));  -- 建在 public
SET search_path = sp1, public;
INSERT INTO t3 VALUES (3, 300);    -- 应解析 sp1.t3（普通表）→ 明文
SELECT * FROM t3 ORDER BY id;      -- 普通表输出 3|300

-- ============================================================
-- 4. 显式限定名不回归
-- ============================================================
INSERT INTO public.t2 VALUES (4, 400);
SELECT id, v FROM public.t2 WHERE id = 4 ORDER BY id;

-- ============================================================
-- 5. SET search_path 切换后缓存失效重解析
-- ============================================================
SET search_path = sp1;
INSERT INTO t_only VALUES (5, 500);
SELECT id, v FROM t_only ORDER BY id;

-- ============================================================
-- 6. 收尾清理
-- ============================================================
RESET search_path;
DROP SCHEMA IF EXISTS sp1 CASCADE;
DROP TABLE IF EXISTS t2 CASCADE;
DROP TABLE IF EXISTS t3 CASCADE;
DROP DEK IF EXISTS dek_pub CASCADE;
DROP DEK IF EXISTS dek_sp1 CASCADE;
DROP KEK IF EXISTS kek_pub CASCADE;
DROP KEK IF EXISTS kek_sp1 CASCADE;
