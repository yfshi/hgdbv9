-- ============================================================
-- kek_dek_drop_he_bound.sql — 删除密钥时须认出「同态密钥绑定」这处占用
--
-- 背景（已修复缺陷）：同态 DEK 不落在 hg_encrypted_columns.column_key_id 上，
--   而是作为 'he_add_dek' / 'he_mul_dek' 条目写进 hg_encrypted_column_args。
--   原占用检查只查 column_key_id，纯同态列该字段为 0 —— 于是「列还在引用着，
--   HE DEK 却能删掉」。删掉后再建一个同名 DEK，分布式身份标识按
--   「模式名.密钥名」哈希得出、与原先完全相同，写入点那条跨 DEK 校验照样通过，
--   但密钥材料已经换了一把，遗留密文只剩密文可读（不报错）。
--
-- 修复：占用检查纳入 he_add_dek/he_mul_dek 绑定，DROP DEK 与 DROP KEK 两条
--   路径都会拒绝，并指出是哪个表的哪一列、用作加还是乘。
--
-- 判定点：同一批密钥在「列仍绑定」与「解绑之后」两个时点的删除结果相反。
--   修复前第 2 节的 he 两问、第 4 节的 hbk_k 三问都会成功（守卫失败）；
--   修复后必须报错。第 3、5 节是反向守卫：未绑定的密钥、以及解绑之后的密钥
--   必须删得掉，避免把占用检查做成一刀切。
--
-- 布局要点：对称列单独挂一把 KEK（hbk_ks），使 hbk_k 这一侧**只**剩同态绑定
--   的 DEK —— 否则 hbk_k 会因为「还有对称绑定的 DEK」而报错，掩盖本条修复是否
--   真的生效（对照跑能看出差异）。对称那一路同时充当既有行为的回归对照。
--
-- 注意：错误 DETAIL 含表 OID，无法跨运行比较，故本用例统一
--   \set VERBOSITY terse，只留 ERROR 主消息。
-- 归属：CREATE KEK/DEK 与 DROP DEK/KEK 都经 libpq C 前端，放 libpq/sql。
--   asym KEK 直接 stat key_path（不做 TECC_KEK_PATH 覆盖），故一律用 %KEKPATH%。
-- ============================================================

\set ON_ERROR_STOP off
\set VERBOSITY terse

-- ============================================================
-- 0. 幂等清理：必须先表后密钥（表在时删密钥正是本条修复要拒绝的动作）
-- ============================================================
DROP TABLE IF EXISTS hbk_t, hbk_q, hbk_s CASCADE;
DROP DEK IF EXISTS hbk_add CASCADE;
DROP DEK IF EXISTS hbk_mul CASCADE;
DROP DEK IF EXISTS hbk_sym CASCADE;
DROP DEK IF EXISTS hbk_free CASCADE;
DROP KEK IF EXISTS hbk_k CASCADE;
DROP KEK IF EXISTS hbk_ks CASCADE;

\! rm -f "$TECC_KEK_PATH/hbk_k.priv" "$TECC_KEK_PATH/hbk_k.pub" 2>/dev/null
\! rm -f "$TECC_KEK_PATH/hbk_ks.key" 2>/dev/null
\! tecc_ktool generate --key-name hbk_k --key-algo sm2 --key-path "$TECC_KEK_PATH" >/dev/null 2>&1
\! tecc_ktool generate --key-name hbk_ks --key-algo sm4 --key-path "$TECC_KEK_PATH" >/dev/null 2>&1

CREATE KEK hbk_k WITH (
    key_store = 'localkms',
    key_algo  = 'sm2',
    key_path  = '%KEKPATH%/hbk_k.priv'
);
CREATE KEK hbk_ks WITH (
    key_store = 'localkms',
    key_algo  = 'sm4',
    key_path  = '%KEKPATH%/hbk_ks.key'
);
CREATE DEK hbk_add  WITH VALUES(KEK = 'hbk_k',  KEY_ALGO = 'he-add');
CREATE DEK hbk_mul  WITH VALUES(KEK = 'hbk_k',  KEY_ALGO = 'he-mul');
CREATE DEK hbk_free WITH VALUES(KEK = 'hbk_k',  KEY_ALGO = 'he-add');
CREATE DEK hbk_sym  WITH VALUES(KEK = 'hbk_ks', KEY_ALGO = 'sm4');

-- ============================================================
-- 1. 三类加密列各自绑定一种密钥（he-add / he-mul / 对称）
-- ============================================================
SELECT '=== 1 setup: bind three kinds of column ===';
CREATE TABLE hbk_t (id int, v int4_enc ENCRYPTED WITH (HE_ADD_DEK = 'hbk_add', SUPPORT_ALGO = 'he-add'));
CREATE TABLE hbk_q (id int, v int4_enc ENCRYPTED WITH (HE_MUL_DEK = 'hbk_mul', SUPPORT_ALGO = 'he-mul'));
CREATE TABLE hbk_s (id int, v text_enc  ENCRYPTED WITH (DEK = 'hbk_sym', SUPPORT_ALGO = 'det'));
INSERT INTO hbk_t VALUES (1, 10);
INSERT INTO hbk_q VALUES (1, 3);
INSERT INTO hbk_s VALUES (1, 'aa');
SELECT '1 he-add col' AS t, v + 5 AS val FROM hbk_t;
SELECT '1 he-mul col' AS t, v * 2 AS val FROM hbk_q;
SELECT '1 det col' AS t, v FROM hbk_s;

-- ============================================================
-- 2. 列仍绑定 → 三个 DROP 都必须报错（修复前 he 的两问会成功）
-- ============================================================
SELECT '=== 2 bound: drop must be rejected ===';
DROP DEK hbk_add;
DROP DEK hbk_mul;
DROP DEK hbk_sym;
-- 三个密钥都还在（修复前这里会是 1：hbk_add/hbk_mul 已被删掉）
SELECT '2 deks alive' AS t, count(*) AS n FROM hg_dek
 WHERE dek_name IN ('hbk_add', 'hbk_mul', 'hbk_sym');

-- ============================================================
-- 3. 反向守卫：没有任何列绑定的 DEK 必须删得掉（不能被一刀切）
-- ============================================================
SELECT '=== 3 unbound: drop must succeed ===';
DROP DEK hbk_free;
SELECT '3 hbk_free alive' AS t, count(*) AS n FROM hg_dek WHERE dek_name = 'hbk_free';

-- ============================================================
-- 4. KEK：其下 DEK 仍被绑定 → 拒绝，且 DROP KEK ... CASCADE 也不能绕过
--    hbk_k 这一侧只剩同态绑定的 DEK（hbk_free 已在上节删除），
--    故修复前这里会成功 —— 是本用例对 HE 路径的第二个判定点。
-- ============================================================
SELECT '=== 4 KEK with bound DEK: drop must be rejected ===';
DROP KEK hbk_k;
DROP KEK hbk_k CASCADE;
DROP KEK hbk_ks;
DROP KEK hbk_ks CASCADE;
SELECT '4 keks alive' AS t, count(*) AS n FROM hg_kek
 WHERE kek_name IN ('hbk_k', 'hbk_ks');

-- ============================================================
-- 5. 解绑之后必须放行：DROP TABLE → DROP DEK → DROP KEK
-- ============================================================
SELECT '=== 5 after unbind: drop must succeed ===';
DROP TABLE hbk_t;
DROP DEK hbk_add;
DROP TABLE hbk_q;
DROP DEK hbk_mul;
DROP TABLE hbk_s;
DROP DEK hbk_sym;
DROP KEK hbk_k;
DROP KEK hbk_ks;
SELECT '5 deks left' AS t, count(*) AS n FROM hg_dek WHERE dek_name LIKE 'hbk\_%';
SELECT '5 keks left' AS t, count(*) AS n FROM hg_kek WHERE kek_name LIKE 'hbk\_%';
