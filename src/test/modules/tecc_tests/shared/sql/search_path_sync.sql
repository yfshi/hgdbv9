--
-- search_path_sync.sql — search_path 缓存与后端状态一致性回归测试
--
-- 背景（Review 问题2 修复）：
--   修复前：前端在后端执行前用字符匹配"预测"SET search_path 并直接写缓存，
--   存在多类与后端不一致（多语句、SET LOCAL/SESSION、RESET、TO DEFAULT、
--   引号值、事务回滚等），导致 KEK/DEK 按错误 schema 解析（找不到或找错）。
--   修复后：后端 search_path 加 GUC_REPORT，每个命令边界上报最终权威值，
--   libpq ParameterStatus 经 sync_guc_params_from_live_conn 惰性同步进缓存
--   （跨语句唯一权威来源）；同串内由前端解析器对 SetStmt 做即时性预测
--   （执行后被权威值覆盖）。
--
-- 判定手段：KEK 名解析走 tecc_cache_get_kek_by_name_search_path（无
-- namespace 兜底），search_path 缓存错误时 CREATE DEK 引用该 KEK 必然报错；
-- 修复后全部成功。修复前各场景（引号值/TO DEFAULT/RESET/回滚/多SET）
-- 均会因缓存与后端不一致而 CREATE DEK 报错。
-- ============================================================

-- ============================================================
-- 0. Setup
--    kek_pub 建在 public（默认 search_path 下创建）；
--    kek_sp1/dek_sp1 建在 sp1（SET search_path = sp1 后创建，
--    服务端按 search_path 归属 KEK/DEK 的 schema）。
-- ============================================================
DROP SCHEMA IF EXISTS sp1 CASCADE;
DROP KEK IF EXISTS kek_pub CASCADE;
DROP KEK IF EXISTS kek_sp1 CASCADE;
DROP DEK IF EXISTS dek_sp1 CASCADE;

-- public 下的 KEK（默认 search_path）
CREATE KEK kek_pub WITH (key_store = 'localkms', key_path = '/tmp/kek_pub.key');

-- sp1 下的 KEK/DEK
CREATE SCHEMA sp1;
SET search_path = sp1;
CREATE KEK kek_sp1 WITH (key_store = 'localkms', key_path = '/tmp/kek_sp1.key');
CREATE DEK dek_sp1 WITH VALUES(KEK = kek_sp1);

-- ============================================================
-- 1. 基线：独立 SET search_path = sp1（简单形式，修复前后均应工作）
-- ============================================================
SET search_path = sp1;
CREATE DEK dek_a WITH VALUES(KEK = kek_sp1);

-- ============================================================
-- 2. 引号值：SET search_path = 'sp1'
--    修复前：文本拦截把带引号的 'sp1' 写入缓存 → current_schema='sp1'
--    （含引号）→ KEK kek_sp1 查找失败 → CREATE DEK 报错。
--    修复后：ParameterStatus 上报规范值 sp1 → 正常。
-- ============================================================
SET search_path = 'sp1';
CREATE DEK dek_b WITH VALUES(KEK = kek_sp1);

-- ============================================================
-- 3. SET search_path TO DEFAULT 后引用 public 的 KEK
--    修复前：缓存写入字面量 "DEFAULT" → current_schema='DEFAULT' →
--    kek_pub（public）查找失败 → CREATE DEK 报错。
--    修复后：ParameterStatus 上报 "$user", public → 正常。
-- ============================================================
SET search_path = sp1;
SET search_path TO DEFAULT;
CREATE DEK dek_c WITH VALUES(KEK = kek_pub);

-- ============================================================
-- 4. RESET search_path 后引用 public 的 KEK
--    修复前：RESET 不被拦截 → 缓存残留 sp1 → kek_pub 查找失败 → 报错。
--    修复后：ParameterStatus 上报恢复值 → 正常。
-- ============================================================
SET search_path = sp1;
RESET search_path;
CREATE DEK dek_d WITH VALUES(KEK = kek_pub);

-- ============================================================
-- 5. 事务回滚后 search_path 恢复：BEGIN; SET; ROLLBACK;
--    修复前：缓存残留 sp1（不随回滚恢复）→ kek_pub 查找失败 → 报错。
--    修复后：AtEOXact 上报回滚后的恢复值 → 正常。
-- ============================================================
BEGIN;
SET search_path = sp1;
ROLLBACK;
CREATE DEK dek_e WITH VALUES(KEK = kek_pub);

-- ============================================================
-- 6. 同串多 SET：SET search_path = public; SET search_path = sp1;
--    修复前：文本拦截只取第一个分号前的值（public）→ 下一语句引用
--    kek_sp1（sp1）查找失败 → 报错。
--    修复后：后端终值 sp1 经 ParameterStatus 上报 → 正常。
-- ============================================================
SET search_path = public; SET search_path = sp1;
CREATE DEK dek_f WITH VALUES(KEK = kek_sp1);

-- ============================================================
-- 7. 同串 SET + 加密 DML（SET 与 DML 同一查询串，依赖解析器对 SetStmt
--    的即时性预测；DEK 名经 search_path 解析；端到端加密/解密验证）
-- ============================================================
SET search_path = sp1;
CREATE TABLE sp1.t1 (id int, v int4_enc ENCRYPTED WITH (DEK = 'dek_sp1', support_algo = 'det'));
SET search_path = sp1; INSERT INTO sp1.t1 VALUES (1, 100); SELECT id, v FROM sp1.t1 ORDER BY id;

-- ============================================================
-- 8. 收尾清理
-- ============================================================
RESET search_path;
DROP SCHEMA IF EXISTS sp1 CASCADE;
DROP TABLE IF EXISTS t_pub CASCADE;
DROP DEK IF EXISTS dek_a;
DROP DEK IF EXISTS dek_b;
DROP DEK IF EXISTS dek_c;
DROP DEK IF EXISTS dek_d;
DROP DEK IF EXISTS dek_e;
DROP DEK IF EXISTS dek_f;
DROP DEK IF EXISTS dek_sp1;
DROP KEK IF EXISTS kek_pub;
DROP KEK IF EXISTS kek_sp1;
