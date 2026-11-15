-- ============================================================================
-- TECC UPDATE 的密文拷贝长度校验（SET 密态列 = 另一个密态列）
-- ============================================================================
-- 覆盖缺口（对齐 libpq 提交 a70e1062d9；libpq 侧同断言见
--     libpq/sql/enc_char_typmod.sql 第 14 节）：
--     UPDATE t5 n SET a = w.w20 FROM t20 w WHERE w.id = 1;
-- 这类赋值由服务端直接把密文搬过去：客户端拿不到源值的真实长度（长度只存在于密文里），
-- 服务端也没有解密能力、密态类型的 typmod 强制转换是空操作 —— 原生那样"按值"判断
-- 做不到。这里退一步按【声明长度】比较：源列声明长度大于目标列声明长度时无法保证
-- 值放得下，直接拒绝。已知代价：原生允许的"值恰好放得下"（第 2 节）也会被拒，
-- 报错文案里写明了原因，原生对照见第 8 节。
--
-- 本用例锁定：
--   1) 源列声明宽于目标列（值超长）→ 拒绝、目标列值不变；
--   2) 源列声明宽于目标列（值其实放得下）→ 同样拒绝（已知代价，固化以防被误判为缺陷）；
--   3) 源列声明不超过目标列 → 放行（写入侧已按声明长度校验过源列的值）；
--   4) 同表整列拷贝（无限定名 / 目标表名限定 / 目标表别名限定）同样受判；
--   5) 源列未声明长度（text_enc、无限定长度的 cast）→ 长度未知，不判、保持原行为；
--   6) 原生对照：同一形态下原生按值判断，放得下的放行、超长的拒绝。
-- ============================================================================

-- ### 环境准备 ###############################################################
\! rm -f "$TECC_KEK_PATH/uccopy_sym.key" 2>/dev/null
\! tecc_ktool generate --key-name uccopy_sym --key-algo sm4 --key-path "$TECC_KEK_PATH" >/dev/null 2>&1

DROP TABLE IF EXISTS uccopy_t CASCADE;
DROP TABLE IF EXISTS uccopy_wide CASCADE;
DROP TABLE IF EXISTS uccopy_same CASCADE;
DROP TABLE IF EXISTS uccopy_self CASCADE;
DROP TABLE IF EXISTS uccopy_text CASCADE;
DROP TABLE IF EXISTS uccopy_np5 CASCADE;
DROP TABLE IF EXISTS uccopy_np20 CASCADE;
DROP DEK IF EXISTS uccopy_dek CASCADE;
DROP KEK IF EXISTS uccopy_kek CASCADE;

CREATE KEK uccopy_kek WITH (
    key_store = 'localkms',
    key_algo = 'sm4',
    key_path = '%KEKPATH%/uccopy_sym.key'
);
CREATE DEK uccopy_dek WITH VALUES (KEK = uccopy_kek);

CREATE TABLE uccopy_t (
    id int,
    v5 varchar_enc(5) ENCRYPTED WITH (DEK = 'uccopy_dek', SUPPORT_ALGO = 'det')
);
CREATE TABLE uccopy_wide (
    id int,
    w20 varchar_enc(20) ENCRYPTED WITH (DEK = 'uccopy_dek', SUPPORT_ALGO = 'det')
);
INSERT INTO uccopy_t VALUES (1, 'abcde');
INSERT INTO uccopy_wide VALUES (1, 'abcdefghij'), (2, 'abc');

-- ### 1. 源声明更宽（值超长）：拒绝 ###########################################
SELECT '=== UCCOPY-1 源声明 20 > 目标 5（值超长）-> 拒绝、值不变（libpq 14.1）===' AS stage;
UPDATE uccopy_t SET v5 = w.w20 FROM uccopy_wide w WHERE w.id = 1;
SELECT v5, v5 = 'abcde' AS eq5, v5 = 'abcdefghij' AS eq10 FROM uccopy_t WHERE id = 1;

-- ### 2. 已知代价：值放得下也一并拒绝 ##########################################
SELECT '=== UCCOPY-2 已知代价：源声明更宽、值其实放得下也一并拒绝（libpq 14.3）===' AS stage;
UPDATE uccopy_t SET v5 = w.w20 FROM uccopy_wide w WHERE w.id = 2;
SELECT v5 FROM uccopy_t WHERE id = 1;

-- ### 3. 源声明不超过目标：放行 ################################################
SELECT '=== UCCOPY-3 源声明不超过目标 -> 放行（libpq 14.2）===' AS stage;
CREATE TABLE uccopy_same (
    id int,
    s5 varchar_enc(5) ENCRYPTED WITH (DEK = 'uccopy_dek', SUPPORT_ALGO = 'det')
);
INSERT INTO uccopy_same VALUES (1, 'vwxyz');
UPDATE uccopy_t SET v5 = s.s5 FROM uccopy_same s WHERE s.id = 1;
SELECT v5, v5 = 'vwxyz' AS eq FROM uccopy_t WHERE id = 1;

-- ### 4. 同表整列拷贝、源更宽：三种限定写法都要拒绝 ############################
SELECT '=== UCCOPY-4 同表整列拷贝、源更宽 -> 拒绝（无限定名 / 目标表名 / 目标表别名）===' AS stage;
CREATE TABLE uccopy_self (
    id int,
    narrow varchar_enc(5)  ENCRYPTED WITH (DEK = 'uccopy_dek', SUPPORT_ALGO = 'det'),
    wide   varchar_enc(20) ENCRYPTED WITH (DEK = 'uccopy_dek', SUPPORT_ALGO = 'det')
);
INSERT INTO uccopy_self VALUES (1, 'abcde', 'abcdefghij');
UPDATE uccopy_self SET narrow = wide WHERE id = 1;
SELECT narrow FROM uccopy_self WHERE id = 1;
UPDATE uccopy_self SET narrow = uccopy_self.wide WHERE id = 1;
SELECT narrow FROM uccopy_self WHERE id = 1;
UPDATE uccopy_self SET narrow = c.wide FROM uccopy_self c WHERE c.id = 1;
SELECT narrow FROM uccopy_self WHERE id = 1;

-- ### 5. 同表整列拷贝、源更窄：放行 ############################################
SELECT '=== UCCOPY-5 同表整列拷贝、源更窄 -> 放行 ===' AS stage;
UPDATE uccopy_self SET wide = narrow WHERE id = 1;
SELECT wide, wide = 'abcde' AS eq FROM uccopy_self WHERE id = 1;

-- ### 6. 源列未声明长度：不判 ##################################################
SELECT '=== UCCOPY-6 源为 text_enc（未声明长度）-> 不判、放行 ===' AS stage;
CREATE TABLE uccopy_text (
    id int,
    t text_enc ENCRYPTED WITH (DEK = 'uccopy_dek', SUPPORT_ALGO = 'det')
);
INSERT INTO uccopy_text VALUES (1, 'abcdefghij');
UPDATE uccopy_t SET v5 = uccopy_text.t FROM uccopy_text WHERE uccopy_text.id = 1;
SELECT v5, v5 = 'abcdefghij' AS eq10 FROM uccopy_t WHERE id = 1;

-- ### 7. 右值为 cast 包裹的列引用：保持原行为 ###################################
SELECT '=== UCCOPY-7 右值为 cast 包裹的列引用 -> 保持原行为（libpq 同样不判）===' AS stage;
UPDATE uccopy_t SET v5 = w.w20::varchar_enc(20) FROM uccopy_wide w WHERE w.id = 2;
SELECT v5, v5 = 'abc' AS eq3 FROM uccopy_t WHERE id = 1;

-- ### 8. 原生对照 #############################################################
SELECT '=== UCCOPY-8 原生对照：按值判断，放得下的放行、超长的拒绝（libpq 14.4）===' AS stage;
CREATE TABLE uccopy_np5  (id int, a varchar(5));
CREATE TABLE uccopy_np20 (id int, w varchar(20));
INSERT INTO uccopy_np5  VALUES (1, 'abcde');
INSERT INTO uccopy_np20 VALUES (1, 'abcdefghij'), (2, 'abc');
UPDATE uccopy_np5 n SET a = w.w FROM uccopy_np20 w WHERE w.id = 1;
SELECT a FROM uccopy_np5 WHERE id = 1;
UPDATE uccopy_np5 n SET a = w.w FROM uccopy_np20 w WHERE w.id = 2;
SELECT a FROM uccopy_np5 WHERE id = 2;

-- ### 清理 ####################################################################
SELECT '=== UCCOPY-END 清理 ===' AS stage;
DROP TABLE IF EXISTS uccopy_t CASCADE;
DROP TABLE IF EXISTS uccopy_wide CASCADE;
DROP TABLE IF EXISTS uccopy_same CASCADE;
DROP TABLE IF EXISTS uccopy_self CASCADE;
DROP TABLE IF EXISTS uccopy_text CASCADE;
DROP TABLE IF EXISTS uccopy_np5 CASCADE;
DROP TABLE IF EXISTS uccopy_np20 CASCADE;
DROP DEK IF EXISTS uccopy_dek CASCADE;
DROP KEK IF EXISTS uccopy_kek CASCADE;

-- ----------------------------------------------------------------------------
-- 两侧输出差异（已由三份 golden 吸收，非缺陷；分类词表见 shared/jdbc_vs_libpq_diff.md）
--   A1 行数页脚：libpq 打 `(1 row)`，runner 不打
--   A4 布尔拼写：libpq `t/f`，runner `true/false`
--   A2 报错前缀：libpq 的 stderr 被归一到输出末尾（`TECC query preprocessing failed: …`），
--      runner 内联为 `ERROR: …`；被拒绝的拷贝赋值在 libpq 侧不产生 stdout 行、runner 侧产生 ERROR 行
--   数据行本身两侧逐字相同（布尔拼写除外）；密文拷贝的报错**核心文案**两侧逐字一致。
-- ----------------------------------------------------------------------------
