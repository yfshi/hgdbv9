-- ============================================================
-- kek_in_dropped_schema.sql — Bug 9332：search_path 指向已删除的模式时
-- 不得创建 KEK / DEK，判定须与原生对象一致
--
-- 背景（TC_IVYSQL_TECC4_SAFETY_ALTER_KEK_007）：
--   create schema sch1; set search_path = sch1; drop schema sch1;
--   CREATE KEK kek_sm4 WITH (key_store=localkms, ...);
-- 步骤3 建表失败、建 KEK 却成功。原因是 PG 的
-- RangeVarGetCreationNamespace()（catalog/namespace.c:497）会拦住
-- "search_path 上没有任何存在的模式"，而 TECC 的 CreateKek/CreateDek
-- 直接取 GetActiveCreationNamespace() 的返回值，未做 OidIsValid 判断，
-- 于是把 key_namespace = 0 写进了 hg_kek。
--
-- 后果（比"创建不该成功"更重）：
--   * 该行成为孤儿：nspname 为 NULL，任何按模式解析的 SQL 路径都看不见它，
--     DROP KEK <裸名> / DROP KEK public.<名> 均报 "does not exist"，永远删不掉；
--   * tecc_kek_key_path_check 的 KEY_PATH 唯一性校验是按路径字符串、且不带
--     模式范围做的，于是这个孤儿把它的密钥文件**永久占死**；
--   * DEK 侧此前靠 generate_distributed_id() 里 get_namespace_name(0) 返回
--     NULL 而报 "namespace with OID 0 does not exist"，属偶然兜底，文案也与
--     KEK/原生对象不一致。
--
-- 修复（src/backend/commands/createkek.c、createdek.c）：未限定名的分支补上
--   if (!OidIsValid(namespaceId)) ereport(ERROR, ERRCODE_UNDEFINED_SCHEMA,
--                                        "no schema has been selected to create in")
-- 与 PG 的 RangeVarGetCreationNamespace 对齐。
--
-- 断言：
--   * 建表 / 建 KEK / 建 DEK 三条路径给出**完全相同**的报错；
--   * 不落库任何 key_namespace = 0 的孤儿行；
--   * key_namespace 恢复可用后，同一个 KEY_PATH 仍能正常建 KEK（文件未被
--     孤儿占死）—— 这是"孤儿不可见且占死文件"这一影响的直接回归点；
--   * 前端侧同一名称解析下裸名建 KEK 后裸名建 DEK 成功（本修复未改变
--     合法路径的行为）。
--
-- 注：第 5 步的 DEK 用限定名 'public.kek_dsch_pub' 指定 KEK —— 前端按
-- current_schema → search_path 顺序解析 KEK 名，而前端 current_schema 是
-- 从 search_path 文本推出来的（不校验模式是否存在），此场景下它推出来的
-- "sch_dsch" 与后端实际值不同。限定名可绕开该解析，使断言只针对后端判定。
-- ============================================================

-- 0. 幂等清理
DROP SCHEMA IF EXISTS sch_dsch CASCADE;
DROP TABLE IF EXISTS t_dsch_probe CASCADE;
DROP DEK IF EXISTS dek_dsch CASCADE;
DROP DEK IF EXISTS dek_dsch_pub CASCADE;
DROP KEK IF EXISTS kek_dsch CASCADE;
DROP KEK IF EXISTS kek_dsch_pub CASCADE;

-- 1. 先在 public 备好一把合法 KEK（第 5 步的 DEK 需要前端能解析到它）
CREATE KEK kek_dsch_pub WITH (
    key_store = 'localkms',
    key_algo = 'sm4',
    key_path = '%KEKPATH%/kek_dsch_pub.key'
);
CREATE DEK dek_dsch_pub WITH VALUES (KEK = 'kek_dsch_pub', KEY_ALGO = 'sm4');
SELECT 'baseline_dek_ok', dek_name FROM hg_dek WHERE dek_name = 'dek_dsch_pub';

-- 2. 报障场景：search_path 指向唯一模式 → 删掉该模式
CREATE SCHEMA sch_dsch;
SET search_path = sch_dsch;
DROP SCHEMA sch_dsch;
SHOW search_path;

-- 3. 对照：原生建表（PG 自己就有这道守卫）
CREATE TABLE t_dsch_probe (a int);

-- 4. 报障点：建 KEK —— 修复前返回 CREATE KEK 并落库 key_namespace = 0
CREATE KEK kek_dsch WITH (
    key_store = 'localkms',
    key_algo = 'sm4',
    key_path = '%KEKPATH%/kek_dsch.key'
);

-- 5. 同族：建 DEK —— 修复前报 "namespace with OID 0 does not exist"
CREATE DEK dek_dsch WITH VALUES (KEK = 'public.kek_dsch_pub', KEY_ALGO = 'sm4');

-- 6. 不落库：既没有名为 kek_dsch/dek_dsch 的行，也没有 key_namespace = 0 的孤儿
SELECT 'kek_dsch_rows', count(*) FROM hg_kek WHERE kek_name = 'kek_dsch';
SELECT 'dek_dsch_rows', count(*) FROM hg_dek WHERE dek_name = 'dek_dsch';
SELECT 'orphan_kek', count(*) FROM hg_kek WHERE key_namespace = 0;
SELECT 'orphan_dek', count(*)
FROM hg_dek d LEFT JOIN pg_namespace n ON d.key_namespace = n.oid
WHERE n.oid IS NULL;

-- 7. 正面回归：search_path 恢复到**存在的**模式后，同一个 KEY_PATH 仍可正常
--    建 KEK —— 证明没有孤儿把它占死（修复前此处会报 "KEY_PATH ... is
--    already used by another KEK"）。
--    注意这里用 "SET search_path = public" 而不是 "RESET"：前端把 current_schema
--    从 search_path 文本推出来，落在无效首元素上时会一直停在那个不存在的名字上
--    （见 bug 记录"举一反三"），会把本步之后所有裸名解析都带偏，与本缺陷无关。
SET search_path = public;
CREATE KEK kek_dsch WITH (
    key_store = 'localkms',
    key_algo = 'sm4',
    key_path = '%KEKPATH%/kek_dsch.key'
);
SELECT 'kek_dsch_after_recovery', n.nspname
FROM hg_kek k JOIN pg_namespace n ON k.key_namespace = n.oid
WHERE k.kek_name = 'kek_dsch';

-- 8. 合法路径行为不变：裸名建 DEK 仍能解析到刚建的 KEK
CREATE DEK dek_dsch WITH VALUES (KEK = 'kek_dsch', KEY_ALGO = 'sm4');
SELECT 'dek_dsch_after_recovery', n.nspname
FROM hg_dek d JOIN pg_namespace n ON d.key_namespace = n.oid
WHERE d.dek_name = 'dek_dsch';

-- 9. 清理
DROP DEK dek_dsch;
DROP DEK dek_dsch_pub;
DROP KEK kek_dsch;
DROP KEK kek_dsch_pub;
SELECT 'cleanup_done';
