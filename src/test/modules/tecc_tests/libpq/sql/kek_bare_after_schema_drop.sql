-- ============================================================
-- kek_bare_after_schema_drop.sql — BUG：search_path 首元素被删后，同一会话里的
-- 裸名 KEK/DEK 操作全部失败（KEK 明明存在）
--
-- 场景（与 Bug 9332 同一入口，但故障在前端）：
--   SET search_path = sch_bnd, public;
--   DROP SCHEMA sch_bnd;                      -- 首元素没了，public 还在
--   CREATE KEK kek_bnd ...;                   -- 后端把 KEK 建在 public
--   CREATE DEK dek_bnd ... (KEK = 'kek_bnd'); -- 修复前：KEK 'kek_bnd' cache miss
--   ALTER KEK kek_bnd ...;                    -- 修复前：Failed to load old KEK
--   （限定名 public.kek_bnd 或重开会话都正常 —— 所以更像是"KEK 丢了"）
--
-- 根因：前端的 current_schema 是从 search_path **文本**推出来的
-- （tecc_cache_manager.c parse_current_schema_from_search_path()，只取第一个
-- 名字，既不校验该模式是否存在，也不问服务端 —— PG 不把 current_schema 作为
-- GUC_REPORT 上报），而后端 recomputeNamespacePath() 会把不存在的模式从
-- activeSearchPath 滤掉，建对象用的是"列表中第一个**存在**的模式"。
-- 于是：
--   * CREATE KEK 的 post 阶段按预测模式（sch_bnd）回查元数据，必然取不到，
--     正式缓存条目没建起来；
--   * 而 stmt_processor 按同一个预测模式插的那条 oid=0 PENDING 条目还挂在那里，
--     它正好挂在 current_schema 名下，后续裸名查找第一步就命中这条假条目。
--
-- 修复：KEK/DEK 的 post-DDL 回查补一层"按 search_path 顺序逐个模式回查"
-- （fe-tecc-integration.c tecc_resolve_{kek,dek}_by_search_path()），规则与表
-- 路径的 tecc_fetch_resolve_table_schema() 一致；命中后按预测模式清 PENDING。
-- 只影响 fail-closed 的失败路径，正常路径不多一次往返。
--
-- 断言：
--   1. 复现前提成立：SHOW search_path 首元素是 sch_bnd，而服务端
--      current_schema() = public（前后端认知不同，这是缺陷的根源）
--   2. 裸名 CREATE KEK 落在 public（后端判定，本修复未改变）
--   3. 裸名 CREATE DEK 成功落在 public（修复前：cache miss）
--   4. 裸名 ALTER KEK 轮换成功，且 DEK 信封被真的重加密（修复前：
--      Failed to load old KEK），轮换后旧数据仍解出明文
--   5. 全程不落 key_namespace = 0 的孤儿行
--
-- 注：第 5 步清理用 "SET search_path = public" 而不是 "RESET"：本用例的场景
-- 就是"search_path 首元素不存在"，RESET 会把它还原成 "\"$user\", public"
-- 后又被前端按文本推导，与本缺陷无关地干扰后续断言。
-- ============================================================

-- 0. 幂等清理
DROP TABLE IF EXISTS t_bnd CASCADE;
DROP TABLE IF EXISTS t_bnd_snap CASCADE;
DROP DEK IF EXISTS dek_bnd CASCADE;
DROP KEK IF EXISTS kek_bnd CASCADE;
DROP SCHEMA IF EXISTS sch_bnd CASCADE;
\! rm -f "$TECC_KEK_PATH/kek_bnd_rot.key" 2>/dev/null

-- 1. 复现前提：首元素是已删除的模式，public 仍在
CREATE SCHEMA sch_bnd;
SET search_path = sch_bnd, public;
DROP SCHEMA sch_bnd;
SHOW search_path;
SELECT 'server_current_schema' AS tag, COALESCE(current_schema(), '(null)') AS cs;

-- 2. 裸名 CREATE KEK：后端落在 public
CREATE KEK kek_bnd WITH (
    key_store = 'localkms',
    key_algo = 'sm4',
    key_path = '%KEKPATH%/kek_bnd.key'
);
SELECT 'kek_ns' AS tag, n.nspname
FROM hg_kek k JOIN pg_namespace n ON k.key_namespace = n.oid
WHERE k.kek_name = 'kek_bnd';

-- 3. 裸名 CREATE DEK：修复前 "KEK 'kek_bnd' cache miss, cannot encrypt DEK"
CREATE DEK dek_bnd WITH VALUES (KEK = 'kek_bnd', KEY_ALGO = 'sm4');
SELECT 'dek_ns' AS tag, n.nspname
FROM hg_dek d JOIN pg_namespace n ON d.key_namespace = n.oid
WHERE d.dek_name = 'dek_bnd';

-- 4. 裸名 ALTER KEK 轮换：修复前 "Failed to load old KEK"
CREATE TABLE t_bnd (
    id  INT4,
    v   TEXT_ENC ENCRYPTED WITH (DEK = 'dek_bnd', SUPPORT_ALGO = 'DET')
);
INSERT INTO t_bnd VALUES (1, 'alpha');
-- 信封快照：只看"轮换后还能查出明文"不够 —— 轮换失败时旧 KEK 原封不动，
-- 数据照样解得出来
CREATE TABLE t_bnd_snap AS
SELECT encode(a.value, 'hex') AS ev
FROM hg_dek_args a JOIN hg_dek d ON a.dek_id = d.oid
WHERE d.dek_name = 'dek_bnd' AND a.key = 'encrypted_value';

\! tecc_ktool generate --key-name kek_bnd_rot --key-algo sm4 --key-path "$TECC_KEK_PATH" >/dev/null 2>&1
ALTER KEK kek_bnd WITH (KEY_PATH = '%KEKPATH%/kek_bnd_rot.key');

SELECT 'dek_reencrypted' AS tag,
       (SELECT ev FROM t_bnd_snap) IS DISTINCT FROM
       (SELECT encode(a.value, 'hex') FROM hg_dek_args a JOIN hg_dek d ON a.dek_id = d.oid
         WHERE d.dek_name = 'dek_bnd' AND a.key = 'encrypted_value') AS reencrypted;
SELECT 'read_after_rotate' AS tag, id, v FROM t_bnd;

-- 5. 不落孤儿行
SELECT 'orphan_kek', count(*) FROM hg_kek WHERE key_namespace = 0;

-- 6. 清理
SET search_path = public;
DROP TABLE t_bnd;
DROP TABLE t_bnd_snap;
DROP DEK dek_bnd;
DROP KEK kek_bnd;
SELECT 'cleanup_done' AS tag;
