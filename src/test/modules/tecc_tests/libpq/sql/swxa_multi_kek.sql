-- ============================================================
-- swxa_multi_kek.sql — 多 KEK：首个 KEK 的 kms_type 不被末个 KEK 的 KEY_STORE 覆盖
-- ============================================================
-- 背景（详见 plan/2026-09-18-KEK元数据多KEK合并-首个KEK-kms_type被覆盖.md）：
-- hg_kek_info 是"每个 KEK 的每个参数各一行"（同一 KEK 的字段列在它的每一行上重复）。
-- 修复前 tecc_parse_kek_list 未按 kek_oid 分组，所有行的参数都并进列表里**第一个**
-- KEK，其中 KEY_STORE 后写覆盖先写 → 首个 KEK 的 kms_type 变成**最后一个** KEK 的
-- 取值：首个是 HSM、末个是 localkms 时，HSM KEK 被当成 $TECC_KEK_PATH 下的本地文件
-- 去加载而报 "Failed to load KEK ... from file 'hsm:id=1'"；同时其余 KEK 在批量列表里
-- 整体丢失（被"按名取单个 KEK"的兜底查询掩盖，故只在首个 KEK 上暴露）。
--
-- 断言：库里同时存在 HSM KEK（名字排序在前）与 localkms KEK（排序在后）时，
--   HSM KEK 仍按硬件路由加解密（INSERT/SELECT 正常），localkms KEK 自身也不受影响。
--
-- 为什么自带一个数据库：KEK 批量查询按**全部 namespace** 取
-- （tecc_cache_manager.c: query.params.batch.filter = NULL），排序是
-- (namespace, kek_name, arg_key) 的全局排序；而回归同一实例上还留着其它用例的 KEK。
-- 只有把这两个 KEK 放进一个干净库，才能确定"谁是第一个、谁是最后一个"——否则缺陷会被
-- 其它 KEK 的排序掩盖，用例变成碰运气。该库用完即删。
-- ============================================================

DROP DATABASE IF EXISTS tecc_mk_db;
CREATE DATABASE tecc_mk_db;
\c tecc_mk_db postgres

-- 1. 只有 1 个 KEK（HSM，名字排序在前）：建 KEK/DEK/表并插入（此步修复前后都应正常）
CREATE KEK hp_k WITH (key_store='swxakms', key_path='hsm:id=1');
CREATE DEK hp_d WITH VALUES (KEK = hp_k);
CREATE TABLE hp_t (id SERIAL, c INT4_ENC ENCRYPTED WITH (DEK = hp_d, SUPPORT_ALGO = 'DET'));
INSERT INTO hp_t (c) VALUES (100);

-- 2. 再加入一个 localkms KEK，名字排序在 hp_k 之后
--    （密钥文件由回归框架的 pregen_localkms_kek_files 依此处的 key_path 预生成）
CREATE KEK zz_local WITH (key_store='localkms', key_path='%KEKPATH%/zz_local.key');

-- 3. 重连：清空连接级 KEK 缓存，使后面不得不走"批量 KEK 列表"重新取元数据。
--    缺陷只在批量列表解析里，若不重连，第 1 步已把 hp_k 正确缓存住，缺陷会被掩盖
--    （本用例最初就是漏了这一步，在修复前的代码上照样通过）。
\c tecc_mk_db postgres

-- 4. 前提自检：排序上 hp_k 在前、zz_local 在后（若此步输出变了，说明前提被破坏，
--    后面的断言就不再覆盖本缺陷，需连同本用例一起复核）
SELECT 'kek order', kek_name FROM hg_kek ORDER BY kek_name;

-- 5. 关键断言：库里存在 2 个 KEK 时，HSM KEK 仍走硬件路由（kms_type 未被 zz_local 覆盖）
INSERT INTO hp_t (c) VALUES (999);
SELECT 'hsm kek after 2nd kek', id, c FROM hp_t ORDER BY id;

-- 6. 对照：localkms KEK 自身加解密正常（同一批元数据里两个 KEK 互不污染）
CREATE DEK zz_d WITH VALUES (KEK = zz_local);
CREATE TABLE zz_t (id INT, c INT4_ENC ENCRYPTED WITH (DEK = zz_d, SUPPORT_ALGO = 'DET'));
INSERT INTO zz_t VALUES (1, 7);
SELECT 'local kek', id, c FROM zz_t;

-- 7. 清理（库用完即删，不留残留影响后续用例）
DROP TABLE hp_t CASCADE;
DROP DEK hp_d CASCADE;
DROP KEK hp_k CASCADE;
DROP TABLE zz_t CASCADE;
DROP DEK zz_d CASCADE;
DROP KEK zz_local CASCADE;
\c postgres postgres
DROP DATABASE tecc_mk_db;
