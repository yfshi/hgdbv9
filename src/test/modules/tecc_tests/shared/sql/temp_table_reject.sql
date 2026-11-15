--
-- temp_table_reject.sql — 临时表限制回归测试
--
-- 背景（Review 遗留说明 C 顺带处理）：
--   临时表 oid 会话私有：数据获取器池连接 JOIN pg_class 查不到本会话临时表，
--   后端登记到全局 hg_encrypted_columns 会产生孤儿行（会话结束自动 drop 不触发
--   依赖清理）。因此明确拒绝"临时加密表"，同时维护会话级临时表清单：
--   - 前端 CREATE TEMP TABLE 记录表名，DROP TABLE 移除；
--   - 未限定表名解析时清单优先（与 PG 的 pg_temp 优先语义一致），
--     避免把临时普通表误解析到 search_path 中的同名加密表。
--
-- 判定手段：
--   a) CREATE TEMP TABLE + 加密列 → 明确报错（后端 DefineRelation 拒绝）；
--   b) 同名遮蔽：本会话 TEMP t3（普通）+ public.t3（加密）→ 未限定访问
--      解析到临时表（明文操作）；DROP TABLE t3 后清单移除 → 重新解析到
--      public.t3（加密操作）。
-- ============================================================

-- ============================================================
-- 0. Setup：public 下 kek_pub/dek_pub
-- ============================================================
DROP KEK IF EXISTS kek_pub CASCADE;
DROP DEK IF EXISTS dek_pub CASCADE;

CREATE KEK kek_pub WITH (key_store = 'localkms', key_path = '/tmp/kek_pub.key');
CREATE DEK dek_pub WITH VALUES(KEK = kek_pub);

-- ============================================================
-- 1. TEMP + 加密列 → 明确拒绝（前端记录错误并透传，后端硬报错）
-- ============================================================
CREATE TEMP TABLE t1 (v int4_enc ENCRYPTED WITH (DEK = 'dek_pub', support_algo = 'det'));

-- ============================================================
-- 2. TEMP + 普通列 → 正常创建
-- ============================================================
CREATE TEMP TABLE t2 (id int, v int);

-- ============================================================
-- 3. 同名遮蔽：本会话 TEMP t3（普通）+ public.t3（加密）
--    未限定访问应解析到临时表（pg_temp 优先）→ 明文操作成功。
--    修复前：查 public.t3 加密列 → 误改写 → 类型报错。
-- ============================================================
CREATE TABLE t3 (id int, v int4_enc ENCRYPTED WITH (DEK = 'dek_pub', support_algo = 'det'));
CREATE TEMP TABLE t3 (id int, v int);
INSERT INTO t3 VALUES (1, 10);
SELECT * FROM t3 ORDER BY id;

-- ============================================================
-- 4. DROP 临时表后清单移除 → 未限定访问重新解析到 public.t3（加密）
-- ============================================================
DROP TABLE t3;
INSERT INTO t3 VALUES (2, 20);
SELECT id, v FROM t3 ORDER BY id;

-- ============================================================
-- 5. 收尾清理
-- ============================================================
DROP TABLE IF EXISTS t3 CASCADE;
DROP TABLE IF EXISTS t2 CASCADE;
DROP DEK IF EXISTS dek_pub CASCADE;
DROP KEK IF EXISTS kek_pub CASCADE;
