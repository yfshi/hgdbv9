-- ============================================================================
-- TECC UPDATE SET 的显式密态 cast：cast 声明的长度必须先生效
-- ============================================================================
-- 覆盖缺口（对齐 libpq 提交 84b3328b54；libpq 侧同断言见
--     libpq/sql/enc_char_typmod.sql 第 13.14 / 13.14b / 13.14c 节）：
--     UPDATE t SET v5 = 'abcdefghij'::varchar_enc(5) ENCRYPTED WITH (...);
-- 原生 PostgreSQL 的显式 cast 是**静默截断**语义（'abcdefghij'::varchar(5) 存 abcde、
-- 不报错）；密态列到服务端时已是密文、服务端无从完成这次截断，因此这一刀必须由
-- 客户端在**加密之前**补上。截断之后仍按【目标列】的声明长度做赋值语义校验
-- （超长且超出部分含非空格 → 拒绝）。
--
-- 本用例锁定：
--   1) cast 声明长度 ≤ 目标列长度 → 按 cast 静默截断后写入成功；
--   2) cast 声明长度 > 目标列长度 → 按目标列拒绝（值不变，绝不静默写入超长值）；
--   3) cast 未声明长度（::varchar_enc）→ 无截断，按目标列拒绝；
--   4) bpchar_enc 与 varchar_enc 同规则；多字节按【字符】计数
--      （按字节计数会把 2 个汉字误判为超长）；
--   5) 同一句里既有带 ENCRYPTED WITH 的 cast、又有写进另一个加密列的裸常量：
--      两者都要被加密（只改其中一条会把另一条明文发往服务端）；
--   6) INSERT 侧同形态是回归锚点（该路径此前已对齐，本次改动不得使它回退）。
-- ============================================================================

-- ### 环境准备 ###############################################################
-- 密钥文件由 tecc_ktool 生成：libpq 侧由 \! 执行，JDBC 侧由回归框架预生成
-- （runner 丢弃 \! 元命令，见 tecc_test_utils.pm 的 pregen_localkms_kek_files）。
\! rm -f "$TECC_KEK_PATH/uscast_sym.key" 2>/dev/null
\! tecc_ktool generate --key-name uscast_sym --key-algo sm4 --key-path "$TECC_KEK_PATH" >/dev/null 2>&1

DROP TABLE IF EXISTS uscast_t CASCADE;
DROP TABLE IF EXISTS uscast_ins CASCADE;
DROP DEK IF EXISTS uscast_dek CASCADE;
DROP KEK IF EXISTS uscast_kek CASCADE;

CREATE KEK uscast_kek WITH (
    key_store = 'localkms',
    key_algo = 'sm4',
    key_path = '%KEKPATH%/uscast_sym.key'
);
CREATE DEK uscast_dek WITH VALUES (KEK = uscast_kek);

CREATE TABLE uscast_t (
    id int,
    v5  varchar_enc(5)  ENCRYPTED WITH (DEK = 'uscast_dek', SUPPORT_ALGO = 'det'),
    v20 varchar_enc(20) ENCRYPTED WITH (DEK = 'uscast_dek', SUPPORT_ALGO = 'det'),
    b3  bpchar_enc(3)   ENCRYPTED WITH (DEK = 'uscast_dek', SUPPORT_ALGO = 'det')
);
INSERT INTO uscast_t VALUES (1, 'abcde', 'x', 'abc');

-- ### 1. 对照：裸常量写入（既有路径） #########################################
SELECT '=== USCAST-1 对照：裸常量写入 varchar_enc(5) ===' AS stage;
SELECT v5, v5 = 'abcde' AS eq FROM uscast_t WHERE id = 1;

-- ### 2. cast(5) 写 varchar_enc(5)：静默截断 ##################################
SELECT '=== USCAST-2 cast(5) 值 10 字符 -> 截断存 abcde（libpq 13.14）===' AS stage;
UPDATE uscast_t SET v5 = 'abcdefghij'::varchar_enc(5) ENCRYPTED WITH (DEK = 'uscast_dek', SUPPORT_ALGO = 'det')
WHERE id = 1;
SELECT v5, v5 = 'abcde' AS eq5, v5 = 'abcdefghij' AS eq10 FROM uscast_t WHERE id = 1;

-- ### 3. cast(20) 写 varchar_enc(5)：按目标列拒绝 #############################
SELECT '=== USCAST-3 cast(20) 值 10 字符 -> 按目标列拒绝、值不变（libpq 13.14b）===' AS stage;
UPDATE uscast_t SET v5 = 'abcdefghij'::varchar_enc(20) ENCRYPTED WITH (DEK = 'uscast_dek', SUPPORT_ALGO = 'det')
WHERE id = 1;
SELECT v5, v5 = 'abcde' AS eq5 FROM uscast_t WHERE id = 1;

-- ### 4. cast 未声明长度：无截断、按目标列拒绝 #################################
SELECT '=== USCAST-4 未声明长度的 cast（::varchar_enc）-> 按目标列拒绝 ===' AS stage;
UPDATE uscast_t SET v5 = 'abcdefghij'::varchar_enc ENCRYPTED WITH (DEK = 'uscast_dek', SUPPORT_ALGO = 'det')
WHERE id = 1;
SELECT v5 FROM uscast_t WHERE id = 1;

-- ### 5. bpchar 同规则 + 多字节按字符计数 ######################################
SELECT '=== USCAST-5a bpchar_enc(3) 值 5 字符 -> 截断存 abc ===' AS stage;
UPDATE uscast_t SET b3 = 'abcde'::bpchar_enc(3) ENCRYPTED WITH (DEK = 'uscast_dek', SUPPORT_ALGO = 'det')
WHERE id = 1;
SELECT b3, b3 = 'abc' AS eq3, b3 = 'abcde' AS eq5 FROM uscast_t WHERE id = 1;

SELECT '=== USCAST-5b cast(2) 写 3 个汉字 -> 按字符计截断为 2 个 ===' AS stage;
UPDATE uscast_t SET v5 = '中文字'::varchar_enc(2) ENCRYPTED WITH (DEK = 'uscast_dek', SUPPORT_ALGO = 'det')
WHERE id = 1;
SELECT v5, v5 = '中文' AS eq2, v5 = '中文字' AS eq3 FROM uscast_t WHERE id = 1;

-- ### 6. 同句混合：cast 与裸常量都要加密 ######################################
SELECT '=== USCAST-6 同句 cast 与裸常量 -> 两者都被加密（libpq 13.14c）===' AS stage;
UPDATE uscast_t SET v5 = 'vwxyz'::varchar_enc(5) ENCRYPTED WITH (DEK = 'uscast_dek', SUPPORT_ALGO = 'det'),
                    v20 = 'qwerty'
WHERE id = 1;
SELECT v5, v5 = 'vwxyz' AS eq5, v20, v20 = 'qwerty' AS eq20 FROM uscast_t WHERE id = 1;

-- ### 7. INSERT 侧回归锚点 ####################################################
SELECT '=== USCAST-7 INSERT 侧同形态（回归锚点）===' AS stage;
CREATE TABLE uscast_ins (
    id int,
    v5 varchar_enc(5) ENCRYPTED WITH (DEK = 'uscast_dek', SUPPORT_ALGO = 'det')
);
INSERT INTO uscast_ins VALUES (1, 'abcdefghij'::varchar_enc(5) ENCRYPTED WITH (DEK = 'uscast_dek', SUPPORT_ALGO = 'det'));
SELECT v5, v5 = 'abcde' AS eq5 FROM uscast_ins WHERE id = 1;

-- ### 8. WHERE / 纯 SELECT 的密态 cast：与列密文口径一致 ########################
-- 背景：上游台账（plan/瀚高全密态数据库-未关闭Bug907.md「待开单 — JDBC 驱动 WHERE / 纯 SELECT 的
--   带 ENCRYPTED WITH cast 与列密文口径不一致」）曾记录 JDBC 侧这两条断言返回 0 / false；
--   2026-09-23 复核确认当前 HEAD 不复现（JDBC 1 / true 与 libpq 1 / t 一致），该条不构成缺口。
--   本节点把这两条断言补进**共享用例**，让双驱动一起锁住，避免口径再次分叉。
SELECT '=== USCAST-8a WHERE 的密态 cast 与裸常量都要命中（对齐 libpq 13.15）===' AS stage;
UPDATE uscast_t SET v5 = 'abcde' WHERE id = 1;
SELECT count(*) AS hit_cast FROM uscast_t
WHERE id = 1 AND v5 = 'abcde'::varchar_enc(5) ENCRYPTED WITH (DEK = 'uscast_dek', SUPPORT_ALGO = 'det');
SELECT count(*) AS hit_plain FROM uscast_t WHERE id = 1 AND v5 = 'abcde';

SELECT '=== USCAST-8b 纯 SELECT 的密态 cast 与列密文等值（对齐 libpq 13.16）===' AS stage;
SELECT 'abcde'::varchar_enc(5) ENCRYPTED WITH (DEK = 'uscast_dek', SUPPORT_ALGO = 'det') = v5 AS eq
FROM uscast_t WHERE id = 1;

-- ### 9. 清理 ##################################################################
SELECT '=== USCAST-END 清理 ===' AS stage;
DROP TABLE IF EXISTS uscast_t CASCADE;
DROP TABLE IF EXISTS uscast_ins CASCADE;
DROP DEK IF EXISTS uscast_dek CASCADE;
DROP KEK IF EXISTS uscast_kek CASCADE;

-- ----------------------------------------------------------------------------
-- 两侧输出差异（已由三份 golden 吸收，非缺陷；分类词表见 shared/jdbc_vs_libpq_diff.md）
--   A1 行数页脚：libpq 打 `(1 row)`，runner 不打
--   A4 布尔拼写：libpq `t/f`，runner `true/false`
--   A2 报错前缀：libpq 的 stderr 被归一到输出末尾（`TECC query preprocessing failed: …`），
--      runner 内联为 `ERROR: …`；被拒绝的 UPDATE 在 libpq 侧不产生 stdout 行、runner 侧产生 ERROR 行
--   数据行本身两侧逐字相同（布尔拼写除外）。
-- ----------------------------------------------------------------------------
