-- ============================================================================
-- TECC INSERT ... SELECT 的「显式 cast 常量」加密
-- ============================================================================
-- 覆盖缺口：SELECT 目标位置对应 INSERT 密态列、且源表达式是"显式 cast 常量"时
--     INSERT INTO t (id, a) SELECT 8, 80::int4_enc;
-- 原先整类 TypeCast 都被明文常量加密分支跳过（该分支只认裸 Const），于是没有
-- 产出改写后的 SQL，报
--     TECC processor detected encrypted columns but failed to produce rewritten query
--
-- 注意 TECC 密态字面量的完整写法是
--     INSERT INTO t (id, a) SELECT 8, 80::int4_enc ENCRYPTED WITH (DEK = 'd');
-- 这类带 ENCRYPTED WITH 的 cast 由 process_one_typecast 依 cast 自带的 DEK
-- 处理，本用例第 4 节把它作为回归对照。不带 ENCRYPTED WITH 的裸 cast 没有
-- DEK 可用，只能与裸常量一致地按「目标列」的 DEK/算法加密——本用例锁定该形态。
--
-- 本用例同时锁定：
--   1) 裸 cast 常量（int4 / varchar / bpchar 带 typmod）可写入并回读明文；
--   2) 替换区间覆盖整段 cast 表达式，密文常量后不残留 "::原始类型" 后缀；
--   3) cast 常量与裸常量、VALUES 两条既有路径写出的密文可互相等值命中；
--   4) 不支持形态（源为明文表列、表达式、列引用、负号 cast 常量）仍明确报错，
--      不会静默把明文写进密态列；
--   5) 同句里带 ENCRYPTED WITH 的 cast 与写进【另一个】加密列的普通常量：两条
--      写入路径都要被加密，不得因其中一条改写成功就整体跳过另一条（第 6 节覆盖
--      INSERT ... VALUES 与 INSERT ... SELECT 两条；WHERE 比较值、纯 SELECT 的
--      同句形态见 libpq/sql/enc_char_typmod.sql 第 13.15 / 13.16 节）。
-- ============================================================================

-- ### 环境准备 ###############################################################
-- HE DEK 必须挂在非对称 KEK（SM2）下，对称 KEK 会被拒绝。
\! rm -f "$TECC_KEK_PATH/kek_isc_sym.key" 2>/dev/null
\! tecc_ktool generate --key-name kek_isc_sym --key-algo sm4 --key-path "$TECC_KEK_PATH" >/dev/null 2>&1
\! rm -f "$TECC_KEK_PATH/kek_isc_asym.priv" "$TECC_KEK_PATH/kek_isc_asym.pub" 2>/dev/null
\! tecc_ktool generate --key-name kek_isc_asym --key-algo sm2 --key-path "$TECC_KEK_PATH" >/dev/null 2>&1

DROP TABLE IF EXISTS isc_test CASCADE;
DROP TABLE IF EXISTS isc_bp CASCADE;
DROP TABLE IF EXISTS isc_he CASCADE;
DROP TABLE IF EXISTS isc_plain CASCADE;
DROP DEK IF EXISTS isc_dek_sym CASCADE;
DROP DEK IF EXISTS isc_dek_add CASCADE;
DROP KEK IF EXISTS isc_kek_sym CASCADE;
DROP KEK IF EXISTS isc_kek_asym CASCADE;

CREATE KEK isc_kek_sym WITH (
    key_store = 'localkms',
    key_algo = 'sm4',
    key_path = '%KEKPATH%/kek_isc_sym.key'
);
CREATE KEK isc_kek_asym WITH (
    key_store = 'localkms',
    key_algo = 'sm2',
    key_path = '%KEKPATH%/kek_isc_asym.priv'
);
CREATE DEK isc_dek_sym WITH VALUES(KEK = isc_kek_sym);
CREATE DEK isc_dek_add WITH VALUES(KEK = isc_kek_asym, KEY_ALGO = 'he-add');

CREATE TABLE isc_test (
    id int,
    a  int4_enc ENCRYPTED WITH (DEK = 'isc_dek_sym', SUPPORT_ALGO = 'det'),
    s  varchar_enc ENCRYPTED WITH (DEK = 'isc_dek_sym', SUPPORT_ALGO = 'det')
);
CREATE TABLE isc_bp (
    id int,
    b  bpchar_enc(10) ENCRYPTED WITH (DEK = 'isc_dek_sym', SUPPORT_ALGO = 'det')
);
CREATE TABLE isc_he (
    id int,
    h  int4_enc ENCRYPTED WITH (HE_ADD_DEK = 'isc_dek_add', SUPPORT_ALGO = 'he-add')
);
CREATE TABLE isc_plain (id int, v int);
INSERT INTO isc_plain VALUES (3, 30);

-- ### 1. 缺陷原形：SELECT 显式 cast 常量 ######################################
\echo '========== 1. INSERT ... SELECT 显式 cast 常量（缺陷原形） =========='
INSERT INTO isc_test (id, a) SELECT 8, 80::int4_enc;
SELECT 'ISC-1-int4', id, a FROM isc_test WHERE id = 8;

INSERT INTO isc_test (id, s) SELECT 9, 'hello'::varchar_enc;
SELECT 'ISC-1-varchar', id, s FROM isc_test WHERE id = 9;

-- 类型名带 typmod：替换区间须吃掉 "(10)"，否则密文常量后会残留 "(10)"
INSERT INTO isc_bp (id, b) SELECT 10, 'xy'::bpchar_enc(10);
SELECT 'ISC-1-bpchar', id, b FROM isc_bp WHERE id = 10;

-- 带符号常量。注意本前端语法表把 UMINUS 的优先级声明在 TYPECAST 之后（与 PG 主
-- 源码的顺序相反），所以 -40::int4_enc 解析为 TypeCast(A_Expr(负号, Const))，
-- 负号落在 cast 内侧 —— 取值与替换区间都必须连符号一起；只取常量本体（ival=40）
-- 会把 -40 写成 +40，而且密文常量后还会残留负号。
INSERT INTO isc_test (id, a) SELECT 11, -40::int4_enc;
SELECT 'ISC-1-neg-cast', id, a FROM isc_test WHERE id = 11;
-- 带符号裸常量（无 cast）
INSERT INTO isc_test (id, a) SELECT 12, -7;
SELECT 'ISC-1-neg-bare', id, a FROM isc_test WHERE id = 12;
-- 显式正号应等价于无符号
INSERT INTO isc_test (id, a) SELECT 13, +50::int4_enc;
SELECT 'ISC-1-plus-cast', id, a FROM isc_test WHERE id = 13;

-- ### 2. 密文有效性：同态列上须产出可继续运算的密文 ############################
\echo '========== 2. he-add 列上的 cast 常量与带符号常量 =========='
INSERT INTO isc_he (id, h) SELECT 11, 11::int4_enc;
SELECT 'ISC-2-read', id, h FROM isc_he WHERE id = 11;
SELECT 'ISC-2-sum', sum(h) FROM isc_he;

-- 带符号常量在同态列上同样要连符号一起加密。若符号丢失，下面几条会写入
-- +60 / +5 / -7，同态和也会随之改变（-47 vs 83），故末尾的和是一个强断言。
INSERT INTO isc_he (id, h) SELECT 12, -60::int4_enc;
SELECT 'ISC-2-neg-cast', id, h FROM isc_he WHERE id = 12;
INSERT INTO isc_he (id, h) SELECT 13, -5;
SELECT 'ISC-2-neg-bare', id, h FROM isc_he WHERE id = 13;
INSERT INTO isc_he (id, h) SELECT 14, +7::int4_enc;
SELECT 'ISC-2-plus-cast', id, h FROM isc_he WHERE id = 14;
SELECT 'ISC-2-sum-signed', sum(h) FROM isc_he;

-- ### 3. 与既有路径的密文互操作 ###############################################
\echo '========== 3. 与裸常量 / VALUES 路径互操作 =========='
-- 同一个值 7 分别经"裸常量 / 裸 cast 常量 / VALUES 裸 cast"写入，
-- DET 等值查询须一次命中三条记录，即三条路径产出的密文格式一致。
INSERT INTO isc_test (id, a) SELECT 20, 7;
INSERT INTO isc_test (id, a) SELECT 21, 7::int4_enc;
INSERT INTO isc_test (id, a) VALUES (22, 7::int4_enc);
SELECT 'ISC-3-hit', count(*) FROM isc_test WHERE a = 7;
-- 带符号常量的密文也必须落在同一个值空间：id=12 写入的是 -7，
-- 若符号在加密时丢失（写成 +7），这条等值查询就命不中。
SELECT 'ISC-3-neg-hit', count(*) FROM isc_test WHERE a = -7;

-- ### 4. 既有能力回归对照 #####################################################
\echo '========== 4. 既有路径不受影响 =========='
-- 带 ENCRYPTED WITH 的 cast：由 process_one_typecast 依 cast 自带 DEK 处理
INSERT INTO isc_test (id, a) SELECT 30, 30::int4_enc ENCRYPTED WITH (DEK = 'isc_dek_sym');
SELECT 'ISC-4-dek-cast', id, a FROM isc_test WHERE id = 30;
-- VALUES 路径的裸 cast（含 typmod）
INSERT INTO isc_bp (id, b) VALUES (31, 'ab'::bpchar_enc(10));
SELECT 'ISC-4-values', id, b FROM isc_bp WHERE id = 31;

-- ### 5. 不支持形态：必须明确报错，不得静默写入明文 ############################
\echo '========== 5. 不支持形态仍明确报错 =========='
-- 源为明文表的列：全密态库不支持把明文列的值写进密态列
INSERT INTO isc_test (id, a) SELECT id, v FROM isc_plain WHERE id = 3;
-- 二元表达式（1 + 2）：不是常量，客户端无法在不解出中间值的前提下加密。
-- 该形态在 VALUES 路径同样被拒，并给出"先算好再作为字面量插入"的指引，属既有的
-- 一致边界，不是本用例所覆盖能力的缺口。
INSERT INTO isc_test (id, a) SELECT 40, 1 + 2;
-- SELECT 自身的列引用
INSERT INTO isc_test (id, a) SELECT 41, id;
-- 上述三句都不得写入。除总数外逐条列出实际落地的行 id 与回读值，
-- 使"哪一条形态被放行、落地的是明文还是密文"在失败时直接可见。
SELECT 'ISC-5-none', count(*) FROM isc_test WHERE id IN (3, 40, 41);
SELECT 'ISC-5-ids', coalesce(string_agg(id::text, ',' ORDER BY id), '(none)')
    FROM isc_test WHERE id IN (3, 40, 41);
SELECT 'ISC-5-read', id, a FROM isc_test WHERE id IN (3, 40, 41, 42) ORDER BY id;

-- ### 6. 同句里带 ENCRYPTED WITH 的 cast 与写进另一加密列的普通常量 ############
\echo '========== 6. cast 与普通常量同句（两条写入路径都要加密） =========='
-- 缺陷原形（2026-09-22 修）：INSERT ... VALUES 分支先跑 cast 处理，它改写成功后
-- 会整体跳过 values 处理器——判据是"rewritten_query 是否非空"。于是同句里写进
-- 另一个加密列的普通常量没人加密，原样发往服务端，服务端按 hex 解析失败：
--     ERROR: invalid hex string: "vwxyz"
--
-- 修法：带 ENCRYPTED WITH 的 cast 不再自己拼一份改写结果，而是登记成
-- is_sql_expression 的 raw value（区间覆盖整段 cast 文本），与普通常量在 phase-2
-- 里一次从左到右改写完成。故本节三个路径都必须两侧都加密：
--   INSERT ... VALUES / INSERT ... SELECT 目标列 / WHERE 比较值。
DROP TABLE IF EXISTS isc_mix CASCADE;
DROP TABLE IF EXISTS isc_mixs CASCADE;
CREATE TABLE isc_mix (
    id int,
    a  varchar_enc(5) ENCRYPTED WITH (DEK = 'isc_dek_sym', SUPPORT_ALGO = 'det'),
    b  varchar_enc(5) ENCRYPTED WITH (DEK = 'isc_dek_sym', SUPPORT_ALGO = 'det')
);

-- 6.1 INSERT ... VALUES：cast 在前、普通常量在后（缺陷原形）
INSERT INTO isc_mix VALUES
    (1, 'abcde'::varchar_enc(5) ENCRYPTED WITH (DEK = 'isc_dek_sym', SUPPORT_ALGO = 'det'), 'vwxyz');
SELECT 'ISC-6-vals-cast-first', id, a, b, a = 'abcde' AS a_ok, b = 'vwxyz' AS b_ok
    FROM isc_mix WHERE id = 1;

-- 6.2 反序：普通常量在前、cast 在后
INSERT INTO isc_mix VALUES
    (2, 'abfgh', 'abcde'::varchar_enc(5) ENCRYPTED WITH (DEK = 'isc_dek_sym', SUPPORT_ALGO = 'det'));
SELECT 'ISC-6-vals-const-first', id, a, b, a = 'abfgh' AS a_ok, b = 'abcde' AS b_ok
    FROM isc_mix WHERE id = 2;

-- 6.3 多行 VALUES：每行都是同句混合形态
INSERT INTO isc_mix VALUES
    (3, 'abijk'::varchar_enc(5) ENCRYPTED WITH (DEK = 'isc_dek_sym', SUPPORT_ALGO = 'det'), 'ablmn'),
    (4, 'abopq', 'abrst'::varchar_enc(5) ENCRYPTED WITH (DEK = 'isc_dek_sym', SUPPORT_ALGO = 'det'));
SELECT 'ISC-6-vals-multi-1', a, b, a = 'abijk' AS a_ok, b = 'ablmn' AS b_ok
    FROM isc_mix WHERE id = 3;
SELECT 'ISC-6-vals-multi-2', a, b, a = 'abopq' AS a_ok, b = 'abrst' AS b_ok
    FROM isc_mix WHERE id = 4;

-- 6.4 对照：只有 cast、无普通常量（其余列为 NULL）
INSERT INTO isc_mix (id, a) VALUES
    (5, 'abuvw'::varchar_enc(5) ENCRYPTED WITH (DEK = 'isc_dek_sym', SUPPORT_ALGO = 'det'));
SELECT 'ISC-6-cast-only', a, a = 'abuvw' AS a_ok FROM isc_mix WHERE id = 5;
-- 6.5 对照：只有普通常量
INSERT INTO isc_mix (id, b) VALUES (6, 'abxyz');
SELECT 'ISC-6-const-only', b, b = 'abxyz' AS b_ok FROM isc_mix WHERE id = 6;
SELECT 'ISC-6-null-count', count(*) FROM isc_mix WHERE a IS NULL OR b IS NULL;

-- 6.6 INSERT ... SELECT：目标列表里的同句混合（走 targetList 分支）
CREATE TABLE isc_mixs (
    a varchar_enc(5) ENCRYPTED WITH (DEK = 'isc_dek_sym', SUPPORT_ALGO = 'det'),
    b varchar_enc(5) ENCRYPTED WITH (DEK = 'isc_dek_sym', SUPPORT_ALGO = 'det')
);
INSERT INTO isc_mixs SELECT
    'abcde'::varchar_enc(5) ENCRYPTED WITH (DEK = 'isc_dek_sym', SUPPORT_ALGO = 'det'), 'vwxyz';
SELECT 'ISC-6-sel-cast-first', a, b, a = 'abcde' AS a_ok, b = 'vwxyz' AS b_ok
    FROM isc_mixs WHERE a = 'abcde';
INSERT INTO isc_mixs SELECT
    'abfgh', 'abcde'::varchar_enc(5) ENCRYPTED WITH (DEK = 'isc_dek_sym', SUPPORT_ALGO = 'det');
SELECT 'ISC-6-sel-const-first', a, b, a = 'abfgh' AS a_ok, b = 'abcde' AS b_ok
    FROM isc_mixs WHERE a = 'abfgh';

-- 6.7 说明：同句混合还有两条路径——WHERE 比较值里的 cast（与裸常量比较值同句）、
-- 纯 SELECT 的 cast，libpq 侧同样已修，但 JDBC 驱动在这两条上口径不同（分别返回
-- 0 行与 false，属 JDBC 侧缺口），故其断言放 libpq 专有用例，避免把该差异固化成
-- 共享期望：见 libpq/sql/enc_char_typmod.sql 第 13.15 / 13.16 节。

-- ### 7. 清理 ################################################################
\echo '========== 7. 清理 =========='
SELECT 'ISC-END';
DROP TABLE isc_mix CASCADE;
DROP TABLE isc_mixs CASCADE;
DROP TABLE isc_test CASCADE;
DROP TABLE isc_bp CASCADE;
DROP TABLE isc_he CASCADE;
DROP TABLE isc_plain CASCADE;
DROP DEK isc_dek_sym CASCADE;
DROP DEK isc_dek_add CASCADE;
DROP KEK isc_kek_sym CASCADE;
DROP KEK isc_kek_asym CASCADE;
