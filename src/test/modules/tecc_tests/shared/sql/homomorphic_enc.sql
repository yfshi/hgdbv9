-- ============================================================
-- homomorphic_enc.sql — 同态加密(HE)全链路回归测试
-- ============================================================
-- 覆盖：EC-ElGamal 加法/减法、有限域 ElGamal 乘法、
--       复合运算、自减(Bug fix)、跨列运算、int2/int4/int8、
--       标量运算(enc±常量/常量±enc/常量*enc，ReqID:TECC4.0 P2-4)
-- ============================================================

-- ============================================================
-- 0. Setup
-- ReqID:TECC4.0 - HE DEK 用非对称 KEK（SM2 信封加密），对称 KEK 会被拒绝。
-- 测试框架设置 TECC_KEK_PATH 覆盖存储目录，但前端 CREATE KEK 校验原始
-- 绝对路径。这里预生成非对称密钥对到固定目录。
-- ============================================================
DROP KEK IF EXISTS kek_he CASCADE;
-- 用 tecc_ktool 生成非对称密钥对（测试框架 PATH 需含 tecc_ktool）
-- 密钥文件写入 $TECC_KEK_PATH（= tmp_check/keys，本回归独立目录），
-- 避免与并行环境共享 /tmp/kek_he_asym.priv（多用户并行回归冲突：
-- 其他用户的 0600 私钥文件会导致本环境读取/覆盖失败）。
-- psql 运行 CWD = tmp_check/tmp，\! 中 $TECC_KEK_PATH 由框架环境注入。
-- key_path 用 %KEKPATH% 占位符（框架替换为本回归独立目录 tmp_check/keys
-- 的绝对路径）：asym KEK 在 CREATE KEK 时直接 stat(key_path) 校验文件存在
-- （不参与 TECC_KEK_PATH 覆盖），必须与 tecc_ktool 生成到 $TECC_KEK_PATH
-- 的文件位置一致，避免并行环境共享 /tmp 下 0600 私钥冲突。
-- tecc_ktool 命名规则：--key-path 为目录，文件名为 <key-name>.priv/.pub。
\! rm -f "$TECC_KEK_PATH/kek_he_asym.priv" "$TECC_KEK_PATH/kek_he_asym.pub" 2>/dev/null
\! tecc_ktool generate --key-name kek_he_asym --key-algo sm2 --key-path "$TECC_KEK_PATH" >/dev/null 2>&1
CREATE KEK kek_he WITH (
	key_store = 'localkms',
	key_algo = 'sm2',
	key_path = '%KEKPATH%/kek_he_asym.priv'
);
-- ReqID:TECC4.0 - HE 硬加密：验证前端传递的 SM2 公钥入库为 65 字节未压缩点
-- （04 前缀 + hex 长度 130），而非 DER 或双重 hex 编码。
SELECT '0_kek_public_key', length(encode(value, 'hex')) = 130
       AND encode(value, 'hex') LIKE '04%' AS pub_ok
FROM hg_kek_args
WHERE kek_id = (SELECT oid FROM hg_kek WHERE kek_name = 'kek_he')
  AND upper(key) = 'PUBLIC_KEY';
CREATE DEK dek_he WITH VALUES(
    KEK = kek_he,
    KEY_ALGO = 'he-add'
);
-- ReqID:TECC4.0 - he-mul 需 FF 密钥对（he-add DEK 仅 EC 密钥对，不能服务 he-mul）
CREATE DEK dek_he_ff WITH VALUES(
    KEK = kek_he,
    KEY_ALGO = 'he-mul'
);

-- ReqID:TECC4.0 - P2-3 对称 KEK/DEK（同态运算结果赋值校验测试用）
-- 幂等：重跑前 DROP（回归实例若复用旧数据目录，避免 already exists）
DROP DEK IF EXISTS dek_sym_he CASCADE;
DROP KEK IF EXISTS kek_sym_he CASCADE;
\! rm -f "$TECC_KEK_PATH/kek_sym.key" 2>/dev/null
\! tecc_ktool generate --key-name kek_sym --key-algo sm4 --key-path "$TECC_KEK_PATH" >/dev/null 2>&1
CREATE KEK kek_sym_he WITH (
    key_store = 'localkms',
    key_algo = 'sm4',
    key_path = '%KEKPATH%/kek_sym.key'
);
CREATE DEK dek_sym_he WITH VALUES(
    KEK = kek_sym_he
);

-- ============================================================
-- 1. 建表 — int2/int4/int8 × (he-add / he-mul / both)
-- ============================================================
SELECT '=== 1 CREATE TABLE ===';

DROP TABLE IF EXISTS he_test CASCADE;
CREATE TABLE he_test (
    id    SERIAL,
    i4_a  INT4_ENC ENCRYPTED WITH (HE_ADD_DEK = 'dek_he', SUPPORT_ALGO = 'he-add'),
    i4_m  INT4_ENC ENCRYPTED WITH (HE_MUL_DEK = 'dek_he_ff', SUPPORT_ALGO = 'he-mul'),
    i4_x  INT4_ENC ENCRYPTED WITH (HE_ADD_DEK = 'dek_he', HE_MUL_DEK = 'dek_he_ff', SUPPORT_ALGO = 'he-add,he-mul'),
    i2_a  INT2_ENC ENCRYPTED WITH (HE_ADD_DEK = 'dek_he', SUPPORT_ALGO = 'he-add'),
    i2_m  INT2_ENC ENCRYPTED WITH (HE_MUL_DEK = 'dek_he_ff', SUPPORT_ALGO = 'he-mul'),
    i8_a  INT8_ENC ENCRYPTED WITH (HE_ADD_DEK = 'dek_he', SUPPORT_ALGO = 'he-add'),
    i8_m  INT8_ENC ENCRYPTED WITH (HE_MUL_DEK = 'dek_he_ff', SUPPORT_ALGO = 'he-mul')
);

-- ============================================================
-- 2. INSERT — 正/负/零
-- ============================================================
SELECT '=== 2 INSERT ===';
INSERT INTO he_test (i4_a, i4_m, i4_x, i2_a, i2_m, i8_a, i8_m)
VALUES (10, 2, 3, 100, 3, 1000, 2);
INSERT INTO he_test (i4_a, i4_m, i4_x, i2_a, i2_m, i8_a, i8_m)
VALUES (100, 5, 7, 200, 4, 2000, 3);
INSERT INTO he_test (i4_a, i4_m, i4_x, i2_a, i2_m, i8_a, i8_m)
VALUES (0, 0, 0, 0, 0, 0, 0);
INSERT INTO he_test (i4_a, i4_m, i4_x, i2_a, i2_m, i8_a, i8_m)
VALUES (-5, -3, -2, -10, -2, -100, -2);

-- ============================================================
-- 3. 轮询解密 — INSERT→SELECT 明文字段验证
-- ============================================================
SELECT '=== 3 round-trip int4 ===';
SELECT id, i4_a, i4_m, i4_x FROM he_test ORDER BY id;

SELECT '=== 3 round-trip int2 ===';
SELECT id, i2_a, i2_m FROM he_test ORDER BY id;

SELECT '=== 3 round-trip int8 ===';
SELECT id, i8_a, i8_m FROM he_test ORDER BY id;

-- ============================================================
-- 4. HE 加法 (EC-ElGamal) — int4_enc + int4_enc
-- ============================================================
SELECT '=== 4 ADD same col ===';
SELECT id, (i4_a + i4_a) IS DISTINCT FROM NULL AS a_self
FROM he_test ORDER BY id;

SELECT '=== 4 ADD cross col ===';
SELECT id, (i4_a + i4_x) IS DISTINCT FROM NULL AS a_cross
FROM he_test ORDER BY id;

-- ============================================================
-- 5. HE 减法 (EC-ElGamal) — int4_enc - int4_enc
--    包含自减(Bug fix: a-a=0序列化无穷远点)
-- ============================================================
SELECT '=== 5 SUB diff ===';
SELECT id, (i4_a - i4_x) IS DISTINCT FROM NULL AS a_sub
FROM he_test ORDER BY id;

SELECT '=== 5 SUB self (zero) ===';
SELECT id, (i4_a - i4_a) IS DISTINCT FROM NULL AS a_self_sub
FROM he_test ORDER BY id;

-- ============================================================
-- 6. HE 乘法 (有限域 ElGamal) — int4_enc * int4_enc
-- ============================================================
SELECT '=== 6 MUL same col ===';
SELECT id, (i4_m * i4_m) IS DISTINCT FROM NULL AS m_self
FROM he_test ORDER BY id;

SELECT '=== 6 MUL cross col ===';
SELECT id, (i4_m * i4_x) IS DISTINCT FROM NULL AS m_cross
FROM he_test ORDER BY id;

-- ============================================================
-- 7. 双算法列 (i4_x) — 同一列测试 ADD/MUL/SUB
-- ============================================================
SELECT '=== 7 x ADD ===';
SELECT id, (i4_x + i4_x) IS DISTINCT FROM NULL AS x_add
FROM he_test ORDER BY id;

SELECT '=== 7 x MUL ===';
SELECT id, (i4_x * i4_x) IS DISTINCT FROM NULL AS x_mul
FROM he_test ORDER BY id;

SELECT '=== 7 x SUB ===';
SELECT id, (i4_x - i4_a) IS DISTINCT FROM NULL AS x_sub
FROM he_test ORDER BY id;

-- ============================================================
-- 8. int2_enc HE 运算
-- ============================================================
SELECT '=== 8 int2 ADD ===';
SELECT id, (i2_a + i2_a) IS DISTINCT FROM NULL AS i2_add
FROM he_test ORDER BY id;

SELECT '=== 8 int2 MUL ===';
SELECT id, (i2_m * i2_m) IS DISTINCT FROM NULL AS i2_mul
FROM he_test ORDER BY id;

-- ============================================================
-- 9. int8_enc HE 运算
-- ============================================================
SELECT '=== 9 int8 ADD ===';
SELECT id, (i8_a + i8_a) IS DISTINCT FROM NULL AS i8_add
FROM he_test ORDER BY id;

SELECT '=== 9 int8 MUL ===';
SELECT id, (i8_m * i8_m) IS DISTINCT FROM NULL AS i8_mul
FROM he_test ORDER BY id;

-- ============================================================
-- 10. 链式运算 — 三元链
-- ============================================================
SELECT '=== 10 chain ADD ===';
SELECT id, (i4_a + i4_a + i4_a) IS DISTINCT FROM NULL AS add3
FROM he_test ORDER BY id;

SELECT '=== 10 chain MUL ===';
SELECT id, (i4_m * i4_m * i4_m) IS DISTINCT FROM NULL AS mul3
FROM he_test ORDER BY id;

-- ============================================================
-- 11. 混合列 (i4_x) 链式运算
-- ============================================================
SELECT '=== 11 x chain ===';
SELECT id,
       (i4_x + i4_x + i4_x) IS DISTINCT FROM NULL AS x_add3,
       (i4_x * i4_x * i4_x) IS DISTINCT FROM NULL AS x_mul3
FROM he_test ORDER BY id;

-- ============================================================
-- 13. HE 标量运算（ReqID:TECC4.0 P2-4）— enc±常量 / 常量±enc / 常量*enc
--     前端对整数常量做同态加密 → 改写为 `enc + 'hex'::intN_enc` →
--     服务端全密文同态操作符(he_add/sub/mul_generic) → 客户端解密明文。
--     （he-add 列的 * 为 EC 标量乘，数学上需明文常量，保留服务端明文路径）
-- ============================================================
SELECT '=== 13 SCALAR ADD a+5 ===';
SELECT id, i4_a + 5 AS s_add FROM he_test ORDER BY id;
SELECT '=== 13 SCALAR SUB a-3 ===';
SELECT id, i4_a - 3 AS s_sub FROM he_test ORDER BY id;
SELECT '=== 13 SCALAR MUL a*3 ===';
SELECT id, i4_a * 3 AS s_mul FROM he_test ORDER BY id;
SELECT '=== 13 SCALAR MUL-L 3*a ===';
SELECT id, 3 * i4_a AS s_mull FROM he_test ORDER BY id;
-- ReqID:TECC4.0 - 负标量乘法（Bug 9250）：he-add 列为 EC 标量乘，负标量须对
-- 每个分块 C1/C2 取反（E(a)*(-k) = E(-k*a)）；旧实现对 k 取绝对值点乘，
-- 导致 a*(-1) 返回 |a|（10/20/30 而非 -10/-20/-30）。
SELECT '=== 13 SCALAR MUL-NEG a*(-1) ===';
SELECT id, i4_a * (-1) AS s_muln1, i4_a * (-2) AS s_muln2,
       i4_a * (-3) AS s_muln3 FROM he_test ORDER BY id;
SELECT '=== 13 SCALAR MUL-NEG-L (-1)*a ===';
SELECT id, (-1) * i4_a AS s_mulnl1, (-2) * i4_a AS s_mulnl2 FROM he_test ORDER BY id;
-- ReqID:TECC4.0 - 负常量表达式（非常量字面量，走服务端明文常量路径，同为 EC 分支）
SELECT '=== 13 SCALAR MUL-NEG-EXPR a*(0-2) ===';
SELECT id, i4_a * (0 - 2) AS s_mulne, (0 - 3) * i4_a AS s_mulnel
FROM he_test ORDER BY id;
-- ReqID:TECC4.0 - int2/int8 he-add 列负标量（同一服务端算子，类型无关）
SELECT '=== 13 SCALAR MUL-NEG int8/int2 ===';
SELECT id, i8_a * (-2) AS s_muln8, i2_a * (-2) AS s_muln2i
FROM he_test ORDER BY id;
-- ReqID:TECC4.0 - 符号一致性：负标量与加法抵消（a*(-1)+a=0）、零标量、大标量
SELECT '=== 13 SCALAR MUL-NEG consistency ===';
SELECT id, i4_a * (-1) + i4_a AS s_mulncan, i4_a * 0 AS s_mulnzero,
       i4_a * (-1000) AS s_mulnbig FROM he_test ORDER BY id;
SELECT '=== 13 SCALAR ADD-L 5+a ===';
SELECT id, 5 + i4_a AS s_addl FROM he_test ORDER BY id;
-- ReqID:TECC4.0 - 常量减密态列：前端把左侧常量同态加密为 E(k)，改写为
-- `'E(k)'::intN_enc - enc`，服务端 he_sub_generic 全密文相减（E(k)−E(a)=E(k−a)）
SELECT '=== 13 SCALAR SUB-L 3-a ===';
SELECT id, 3 - i4_a AS s_subl FROM he_test ORDER BY id;
SELECT '=== 13 SCALAR SUB-L 30-a ===';
SELECT id, 30 - i4_a AS s_subl2 FROM he_test ORDER BY id;
SELECT '=== 13 SCALAR SUB-L -3-a ===';
SELECT id, -3 - i4_a AS s_subln FROM he_test ORDER BY id;
SELECT '=== 13 SCALAR SUB-L int8 3-i8a ===';
SELECT id, 3 - i8_a AS s_subl8 FROM he_test ORDER BY id;
-- ReqID:TECC4.0 - 一元负号与服务端左常量减（新增操作符）：
--   `-enc` → 服务端一元 `-(intN_enc)`（EC 对 C2 点取反 / FF 对 c2 取负）；
--   `(3*2) - enc` → 常量表达式在左，前端透传 → 服务端 `-(intN,intN_enc)`
SELECT '=== 13 UNARY NEG -i4_a ===';
SELECT id, -i4_a AS s_uneg FROM he_test ORDER BY id;
SELECT '=== 13 UNARY NEG -i8_a ===';
SELECT id, -i8_a AS s_uneg8 FROM he_test ORDER BY id;
SELECT '=== 13 UNARY NEG -i2_a ===';
SELECT id, -i2_a AS s_uneg2 FROM he_test ORDER BY id;
SELECT '=== 13 UNARY NEG -i4_a+3 ===';
SELECT id, -i4_a + 3 AS s_unegp FROM he_test ORDER BY id;
SELECT '=== 13 SUB-L-EXPR (3*2)-i4_a ===';
SELECT id, (3 * 2) - i4_a AS s_suble FROM he_test ORDER BY id;
-- ReqID:TECC4.0 - 复合表达式左侧常量（方案 B 服务端 `-(intN,intN_enc)` 兜底）：
--   内层 `a + 1` 前端加密改写为全密文加法，外层 `k - <密文>` 由服务端持公钥
--   现场加密常量后相减；常量明文不出前端的目标属方案 A（安全增强），见
--   plan/2026-09-01-HE复合表达式左常量-修复方案.md §6.4
SELECT '=== 13 SUB-L-COMP 10-(a+1) ===';
SELECT id, 10 - (i4_a + 1) AS s_sublc FROM he_test ORDER BY id;
SELECT '=== 13 SUB-L-COMP 10-a*2 ===';
SELECT id, 10 - i4_a * 2 AS s_sublc2 FROM he_test ORDER BY id;
SELECT '=== 13 SUB-L-COMP 100-(a+10)*2 ===';
SELECT id, 100 - (i4_a + 10) * 2 AS s_sublc3 FROM he_test ORDER BY id;
SELECT '=== 13 SCALAR NEG a+(-5) ===';
SELECT id, i4_a + (-5) AS s_neg FROM he_test ORDER BY id;
SELECT '=== 13 SCALAR SUB-NEG a-(-5) ===';
SELECT id, i4_a - (-5) AS s_subneg FROM he_test ORDER BY id;
SELECT '=== 13 SCALAR COMPOUND a+3*2 ===';
SELECT id, i4_a + 3 * 2 AS s_comp FROM he_test ORDER BY id;
SELECT '=== 13 SCALAR int8 b+50 ===';
SELECT id, i8_a + 50 AS s_i8 FROM he_test ORDER BY id;
-- ReqID:TECC4.0 - he-mul 列标量乘：常量前端 FF-ElGamal 加密（EG_MUL 段），
-- 服务端 he_mul_generic 全密文相乘，常量明文不出前端
SELECT '=== 13 SCALAR MUL-M m*3 ===';
SELECT id, i4_m * 3 AS s_mulm FROM he_test ORDER BY id;
SELECT '=== 13 SCALAR MUL-ML 3*m ===';
SELECT id, 3 * i4_m AS s_mulml FROM he_test ORDER BY id;
SELECT '=== 13 SCALAR MUL-MNEG m*(-2) ===';
SELECT id, i4_m * (-2) AS s_mulmn FROM he_test ORDER BY id;
-- ReqID:TECC4.0 - he-mul 负常量表达式（非常量字面量 → 服务端 he_mul_scalar_core
-- 的 FF 分支，常量在服务端现场加密；同分支旧实现同样对 k 取绝对值丢符号）
SELECT '=== 13 SCALAR MUL-MNEG-EXPR m*(0-2) ===';
SELECT id, i4_m * (0 - 2) AS s_mulmne, (0 - 3) * i4_m AS s_mulmnel
FROM he_test ORDER BY id;
SELECT '=== 13 SCALAR MUL-MZERO m*0 ===';
SELECT id, i4_m * 0 AS s_mulmz FROM he_test ORDER BY id;
-- ReqID:TECC4.0 - int2/int8 标量（常量按列加密类型改写）
SELECT '=== 13 SCALAR int2 a+5 ===';
SELECT id, i2_a + 5 AS s_i2 FROM he_test ORDER BY id;
SELECT '=== 13 SCALAR int8 MUL b*3 ===';
SELECT id, i8_m * 3 AS s_i8m FROM he_test ORDER BY id;

-- ============================================================
-- 14. 同态运算结果赋值校验（ReqID:TECC4.0 P2-3）
--     运算结果仅含 HE 密文段，只允许赋给纯 he-add/he-mul 列；
--     赋给含对称算法（DET/OPE/SEARCH）的列报错（需求 V4.0.1-01-3）
-- ============================================================
DROP TABLE IF EXISTS t_assign CASCADE;
CREATE TABLE t_assign (
    id SERIAL,
    d INT4_ENC ENCRYPTED WITH (DEK = 'dek_sym_he', SUPPORT_ALGO = 'det'),
    a INT4_ENC ENCRYPTED WITH (HE_ADD_DEK = 'dek_he', SUPPORT_ALGO = 'he-add'),
    x INT4_ENC ENCRYPTED WITH (DEK = 'dek_sym_he', HE_ADD_DEK = 'dek_he', SUPPORT_ALGO = 'det,he-add')
);
INSERT INTO t_assign (d, a, x) VALUES (1, 10, 100);
SELECT '=== 14 assign OK he-add col ===';
UPDATE t_assign SET a = a + a WHERE id = 1;
SELECT a FROM t_assign WHERE id = 1;
SELECT '=== 14 assign ERR symmetric col ===';
UPDATE t_assign SET d = a + a WHERE id = 1;
SELECT '=== 14 assign ERR symmetric+he col ===';
UPDATE t_assign SET x = a + a WHERE id = 1;

-- ============================================================
-- 15. SUM 聚合（需求 01-1：he-add 列支持 SUM 聚合）
--     服务端聚合密文 → 客户端解密 sum 结果
-- ============================================================
SELECT '=== 15 SUM int4 ===';
SELECT sum(i4_a) AS s_a FROM he_test;
SELECT '=== 15 SUM int8 ===';
SELECT sum(i8_a) AS s_i8 FROM he_test;
SELECT '=== 15 SUM both ===';
SELECT sum(i4_x) AS s_x FROM he_test;
SELECT '=== 15 SUM int2 ===';
SELECT sum(i2_a) AS s_i2 FROM he_test;
-- ReqID:TECC4.0 - SUM 聚合结果负标量乘（Bug 9250 同族：聚合密文再标量乘）
SELECT '=== 15 SUM neg scalar ===';
SELECT sum(i4_a) * (-1) AS s_sum_n1, sum(i4_a) * (-2) AS s_sum_n2 FROM he_test;

-- ============================================================
-- 16. 明文正确性 — 列间运算结果解密为明文并校验值
-- ============================================================
SELECT '=== 16 PT ADD cross ===';
SELECT id, i4_a + i4_x AS a_plus_x FROM he_test ORDER BY id;
SELECT '=== 16 PT SUB cross ===';
SELECT id, i4_a - i4_x AS a_sub_x FROM he_test ORDER BY id;
SELECT '=== 16 PT MUL cross ===';
SELECT id, i4_m * i4_x AS m_mul_x FROM he_test ORDER BY id;
SELECT '=== 16 PT chain add ===';
SELECT id, i4_a + i4_a + i4_x AS chain FROM he_test ORDER BY id;
SELECT '=== 16 PT scalar MUL on add col (EC k-fold) ===';
SELECT id, i4_a * 3 AS a3 FROM he_test ORDER BY id;

-- ============================================================
-- 17. 负向测试 — 缺段/跨 DEK 运算报错
--     跨 DEK（he-add 列 dek_he vs he-mul 列 dek_he_ff）运算前端拦截；
--     混合运算（* 结果仅 EG_MUL 段再 + 需 EG_ADD）报缺段错误
-- ============================================================
SELECT '=== 17 ERR add col MUL ===';
SELECT i4_a * i4_m FROM he_test;
SELECT '=== 17 ERR mul col ADD ===';
SELECT i4_m + i4_a FROM he_test;
SELECT '=== 17 ERR mix both col ===';
SELECT i4_x * i4_x + i4_x FROM he_test;

-- ============================================================
-- 18. WHERE 子句 HE 标量运算（需求 01-1 标量；前端透传→服务端）
-- ============================================================
SELECT '=== 18 WHERE scalar add ===';
SELECT id, i4_a FROM he_test WHERE i4_a + 5 IS NOT NULL ORDER BY id;
SELECT '=== 18 ERR WHERE scalar compare (HE ciphertext not comparable) ===';
SELECT id FROM he_test WHERE i4_a + 5 > 10 ORDER BY id;

-- ============================================================
-- 19. UPDATE SET 标量赋值（需求 01-1：运算结果赋值）
-- ============================================================
SELECT '=== 19 UPDATE scalar ===';
UPDATE he_test SET i4_a = i4_a + 100 WHERE id = 1;
SELECT id, i4_a FROM he_test WHERE id = 1;
SELECT '=== 19 UPDATE sub scalar ===';
UPDATE he_test SET i4_a = i4_a - 50 WHERE id = 2;
SELECT id, i4_a FROM he_test WHERE id = 2;
-- ReqID:TECC4.0 - 混合 SET：A_Expr 标量（常量前端加密）+ 常量赋值并存
-- （修复 values 处理器对二元 A_Expr 创建占位值导致 SQL 损坏的问题）。
-- 用独立表，避免扰动 he_test 后续零边界/跨类型断言的测试数据。
SELECT '=== 19 UPDATE mixed A_Expr + const ===';
DROP TABLE IF EXISTS t_set_mix CASCADE;
CREATE TABLE t_set_mix (
    id SERIAL,
    a INT4_ENC ENCRYPTED WITH (HE_ADD_DEK = 'dek_he', SUPPORT_ALGO = 'he-add'),
    m INT4_ENC ENCRYPTED WITH (HE_MUL_DEK = 'dek_he_ff', SUPPORT_ALGO = 'he-mul')
);
INSERT INTO t_set_mix (a, m) VALUES (0, 2);
UPDATE t_set_mix SET a = a + 100, m = 5 WHERE id = 1;
SELECT id, a, m FROM t_set_mix WHERE id = 1;
SELECT '=== 19 UPDATE mul scalar (he-mul) ===';
UPDATE t_set_mix SET m = m * 3 WHERE id = 1;
SELECT id, m FROM t_set_mix WHERE id = 1;
-- ReqID:TECC4.0 - UPDATE SET 负标量乘（Bug 9250 的写入路径：结果密文回写 he-add 列）
SELECT '=== 19 UPDATE mul neg scalar (he-add) ===';
UPDATE t_set_mix SET a = a * (-2) WHERE id = 1;
SELECT id, a, m FROM t_set_mix WHERE id = 1;
DROP TABLE t_set_mix CASCADE;

-- ============================================================
-- 20. 建表/建DEK 校验错误场景（需求 4.4.1.2 校验规则，P1-1/P1-2）
--     错误用例报错后 psql 继续执行
-- ============================================================
SELECT '=== 20 ERR missing sym DEK ===';
CREATE TABLE t_bad1 (a int4_enc ENCRYPTED WITH (HE_ADD_DEK = 'dek_he', SUPPORT_ALGO = 'det,he-add'));
SELECT '=== 20 ERR HE_ADD_DEK points sm4 ===';
CREATE TABLE t_bad2 (a int4_enc ENCRYPTED WITH (DEK = 'dek_sym_he', HE_ADD_DEK = 'dek_sym_he', SUPPORT_ALGO = 'det,he-add'));
SELECT '=== 20 ERR sym DEK for HE-only ===';
CREATE TABLE t_bad3 (a int4_enc ENCRYPTED WITH (DEK = 'dek_sym_he', SUPPORT_ALGO = 'he-add'));
SELECT '=== 20 ERR HE DEK with sym KEK ===';
CREATE DEK dek_bad_he WITH VALUES(KEK = kek_sym_he, KEY_ALGO = 'he-add');

-- ============================================================
-- 21. 边界 — 零参与运算（服务端同态）
-- ============================================================
SELECT '=== 21 ADD zero ===';
SELECT id, i4_a + 0 AS a0 FROM he_test ORDER BY id;
SELECT '=== 21 SUB zero ===';
SELECT id, i4_a - 0 AS s0 FROM he_test ORDER BY id;
SELECT '=== 21 MUL zero ===';
SELECT id, i4_m * 0 AS m0 FROM he_test ORDER BY id;

-- ============================================================
-- 22. 跨类型运算（int2/int4/int8 混合，同一DEK；需求 01-1/01-2）
--     结果类型取较大者：int2+int4→int4_enc、int2+int8→int8_enc、int4+int8→int8_enc
-- ============================================================
SELECT '=== 22 CT add int2+int4 ===';
SELECT id, i2_a + i4_a AS r24 FROM he_test ORDER BY id;
SELECT '=== 22 CT add int2+int8 ===';
SELECT id, i2_a + i8_a AS r28 FROM he_test ORDER BY id;
SELECT '=== 22 CT add int4+int8 ===';
SELECT id, i4_a + i8_a AS r48 FROM he_test ORDER BY id;
SELECT '=== 22 CT sub int4-int2 ===';
SELECT id, i4_a - i2_a AS s42 FROM he_test ORDER BY id;
SELECT '=== 22 CT mul int2*int4 ===';
SELECT id, i2_m * i4_m AS m24 FROM he_test ORDER BY id;
SELECT '=== 22 CT mul int2*int8 ===';
SELECT id, i2_m * i8_m AS m28 FROM he_test ORDER BY id;
SELECT '=== 22 CT mul int4*int8 ===';
SELECT id, i4_m * i8_m AS m48 FROM he_test ORDER BY id;
-- 反向方向（验证 18 个跨类型操作符全部注册可用，结果类型仍取较大者）
SELECT '=== 22 CT add int4+int2 reverse ===';
SELECT id, i4_a + i2_a AS r42 FROM he_test ORDER BY id;
SELECT '=== 22 CT add int8+int2 reverse ===';
SELECT id, i8_a + i2_a AS r82 FROM he_test ORDER BY id;
SELECT '=== 22 CT add int8+int4 reverse ===';
SELECT id, i8_a + i4_a AS r84 FROM he_test ORDER BY id;
SELECT '=== 22 CT sub int2-int4 ===';
SELECT id, i2_a - i4_a AS s24 FROM he_test ORDER BY id;
SELECT '=== 22 CT sub int8-int2 ===';
SELECT id, i8_a - i2_a AS s82 FROM he_test ORDER BY id;
SELECT '=== 22 CT sub int8-int4 ===';
SELECT id, i8_a - i4_a AS s84 FROM he_test ORDER BY id;
SELECT '=== 22 CT mul int4*int2 reverse ===';
SELECT id, i4_m * i2_m AS m42 FROM he_test ORDER BY id;
SELECT '=== 22 CT mul int8*int2 reverse ===';
SELECT id, i8_m * i2_m AS m82 FROM he_test ORDER BY id;
SELECT '=== 22 CT mul int8*int4 reverse ===';
SELECT id, i8_m * i4_m AS m84 FROM he_test ORDER BY id;

-- ============================================================
-- 22b. 跨类型边界值（各类型最大/最小值参与，避免溢出组合）
--      边界行定位用普通列 id 过滤（HE 列无 DET/OPE 不可等值比较）
-- ============================================================
SELECT '=== 22 CT boundary table ===';
DROP TABLE IF EXISTS he_ct;
CREATE TABLE he_ct (
    id INT,
    i2 INT2_ENC ENCRYPTED WITH (HE_ADD_DEK = 'dek_he', SUPPORT_ALGO = 'he-add'),
    i4 INT4_ENC ENCRYPTED WITH (HE_ADD_DEK = 'dek_he', SUPPORT_ALGO = 'he-add'),
    i8 INT8_ENC ENCRYPTED WITH (HE_ADD_DEK = 'dek_he', SUPPORT_ALGO = 'he-add')
);
INSERT INTO he_ct VALUES (1, 32767, 1000, 2000);
INSERT INTO he_ct VALUES (2, -32768, 1000, 2000);
INSERT INTO he_ct VALUES (3, 5, 2147483647, 9223372036854775807);
INSERT INTO he_ct VALUES (4, 5, -2147483648, -9223372036854775808);
SELECT '=== 22 CT boundary int2 max ===';
SELECT i2 + i4 AS b24, i2 + i8 AS b28 FROM he_ct WHERE id = 1;
SELECT '=== 22 CT boundary int2 min ===';
SELECT i2 + i4 AS b24m2, i2 + i8 AS b28m2 FROM he_ct WHERE id = 2;
SELECT '=== 22 CT boundary int4 max ===';
SELECT i2 + i4 AS b24m, i2 + i8 AS b28m FROM he_ct WHERE id = 3;
SELECT '=== 22 CT boundary int4 min ===';
SELECT i2 + i4 AS b24n FROM he_ct WHERE id = 4;
SELECT '=== 22 CT boundary int8 min ===';
SELECT i2 + i8 AS b28n FROM he_ct WHERE id = 4;
SELECT '=== 22 CT boundary int8 max + int4 max ===';
SELECT i4 + i8 AS b48m FROM he_ct WHERE id = 3;
DROP TABLE he_ct;

-- ============================================================
-- 23. HE 密钥材料存储断言（ReqID:TECC4.0 密钥管理）
--     CREATE DEK 时为 per-DEK 生成密钥对并注入 hg_dek_args：
--       he-add → EC-ElGamal ：he_ec_pubkey  = 65B 非压缩点（hex 130）
--                             he_ec_privkey = SM2 信封（OpenSSL ASN.1）
--       he-mul → FF ElGamal ：he_eg_pubkey  = 2048-bit h（hex 512）
--                             he_eg_privkey = SM2 信封（OpenSSL ASN.1）
--     私钥信封用 KEK 的 SM2 公钥加密（scheme B）。长度不固定（ASN.1
--     INTEGER 前导零）：EC 信封 hex 约 278-282、FF 约 726-736，用稳定
--     范围断言——远大于明文 hex（64/512）且远离 SM4-CBC+HMAC（160/608，
--     明文 32/256B 时的固定 16 倍数长度）。value 存 ASCII hex 字符串，
--     length(encode(value,'escape')) 即 hex 字符数。
-- ============================================================
SELECT '=== 23 HE key material ===';

-- 23.1 dek_he（KEY_ALGO = 'he-add'）仅 EC 密钥对
SELECT '23_ec_pub', length(encode(value, 'escape')) = 130 AS ec_pub_ok
FROM hg_dek_args
WHERE dek_id = (SELECT oid FROM hg_dek WHERE dek_name = 'dek_he')
  AND lower(key) = 'he_ec_pubkey';
SELECT '23_ec_priv_env', length(encode(value, 'escape')) BETWEEN 260 AND 300 AS ec_priv_env_ok
FROM hg_dek_args
WHERE dek_id = (SELECT oid FROM hg_dek WHERE dek_name = 'dek_he')
  AND lower(key) = 'he_ec_privkey';
SELECT '23_ec_keytype', encode(value, 'escape') = 'he-add' AS kt_ok
FROM hg_dek_args
WHERE dek_id = (SELECT oid FROM hg_dek WHERE dek_name = 'dek_he')
  AND lower(key) = 'key_algo';
SELECT '23_ec_no_eg', count(*) = 0 AS no_eg_ok
FROM hg_dek_args
WHERE dek_id = (SELECT oid FROM hg_dek WHERE dek_name = 'dek_he')
  AND lower(key) IN ('he_eg_pubkey', 'he_eg_privkey');

-- 23.2 he-mul DEK（KEY_ALGO = 'he-mul'）FF 密钥对
-- （dek_he_ff 已在文件开头创建，供 he_test 表 he-mul 列使用；此处仅验证密钥材料）
SELECT '23_eg_pub', length(encode(value, 'escape')) = 512 AS eg_pub_ok
FROM hg_dek_args
WHERE dek_id = (SELECT oid FROM hg_dek WHERE dek_name = 'dek_he_ff')
  AND lower(key) = 'he_eg_pubkey';
SELECT '23_eg_priv_env', length(encode(value, 'escape')) BETWEEN 700 AND 800 AS eg_priv_env_ok
FROM hg_dek_args
WHERE dek_id = (SELECT oid FROM hg_dek WHERE dek_name = 'dek_he_ff')
  AND lower(key) = 'he_eg_privkey';
SELECT '23_eg_keytype', encode(value, 'escape') = 'he-mul' AS kt_ok
FROM hg_dek_args
WHERE dek_id = (SELECT oid FROM hg_dek WHERE dek_name = 'dek_he_ff')
  AND lower(key) = 'key_algo';
SELECT '23_eg_no_ec', count(*) = 0 AS no_ec_ok
FROM hg_dek_args
WHERE dek_id = (SELECT oid FROM hg_dek WHERE dek_name = 'dek_he_ff')
  AND lower(key) IN ('he_ec_pubkey', 'he_ec_privkey');

-- 23.3 FF DEK 全链路 round-trip（he-mul 列用 per-DEK FF 密钥）
DROP TABLE IF EXISTS t_he_ff CASCADE;
CREATE TABLE t_he_ff(id serial, m INT4_ENC ENCRYPTED WITH (HE_MUL_DEK = 'dek_he_ff', SUPPORT_ALGO = 'he-mul'));
INSERT INTO t_he_ff(m) VALUES (7), (-9);
SELECT '23_ff_roundtrip', id, m FROM t_he_ff ORDER BY id;
DROP TABLE t_he_ff CASCADE;
-- （dek_he_ff 保留：he_test 表 he-mul 列与后续 24 段幂运算仍需使用）

-- ============================================================
-- 24. 幂运算（^ / power，仅 he-mul 列：重复 FF-ElGamal 乘法）
-- ============================================================
SELECT '=== 24 pow i4_m ^ 2 ===';
SELECT id, i4_m ^ 2 AS p2 FROM he_test ORDER BY id;
SELECT '=== 24 pow i4_m ^ 3 ===';
SELECT id, i4_m ^ 3 AS p3 FROM he_test ORDER BY id;
SELECT '=== 24 pow power(i4_m, 3) ===';
SELECT id, power(i4_m, 3) AS pp FROM he_test ORDER BY id;
SELECT '=== 24 pow i2_m ^ 2 ===';
SELECT id, i2_m ^ 2 AS p2_2 FROM he_test ORDER BY id;
SELECT '=== 24 pow i8_m ^ 2 ===';
SELECT id, i8_m ^ 2 AS p2_8 FROM he_test ORDER BY id;
SELECT '=== 24 pow ERR he-add column (lacks EG_MUL) ===';
SELECT i4_a ^ 2 FROM he_test LIMIT 1;
SELECT '=== 24 pow ERR exponent <= 0 ===';
SELECT i4_m ^ 0 FROM he_test LIMIT 1;
SELECT '=== 24 pow ERR exponent > max (10000) ===';
SELECT i4_m ^ 10001 FROM he_test LIMIT 1;

-- ============================================================
-- 24b. 幂运算结果类型验证（pg_typeof → int8_enc）
-- ============================================================
SELECT '=== 24b pow type: i2_m ^ 2 → int8_enc ===';
SELECT pg_typeof(i2_m ^ 2) AS type FROM he_test LIMIT 1;
SELECT '=== 24b pow type: i4_m ^ 2 → int8_enc ===';
SELECT pg_typeof(i4_m ^ 2) AS type FROM he_test LIMIT 1;
SELECT '=== 24b pow type: i8_m ^ 2 → int8_enc ===';
SELECT pg_typeof(i8_m ^ 2) AS type FROM he_test LIMIT 1;
SELECT '=== 24b pow type: power(i2_m, 3) → int8_enc ===';
SELECT pg_typeof(power(i2_m, 3)) AS type FROM he_test LIMIT 1;
SELECT '=== 24b pow type: power(i4_m, 3) → int8_enc ===';
SELECT pg_typeof(power(i4_m, 3)) AS type FROM he_test LIMIT 1;
SELECT '=== 24b pow type: power(i8_m, 3) → int8_enc ===';
SELECT pg_typeof(power(i8_m, 3)) AS type FROM he_test LIMIT 1;

-- ============================================================
-- 25. 一列多 DEK 混合配置（ReqID:TECC4.0 P0-2 联调验证）
--     对称 DEK（det/ope/search）+ HE DEK（he-add/he-mul）混合，
--     验证多 DEK 列 INSERT 加密 / SELECT 解密 / 对称谓词 / HE 运算。
--     覆盖 P0-2（hg_encrypted_column_args 存储 he_add_dek/he_mul_dek
--     绑定）+ P2-1（解密按段内 DEK ID 反查私钥）端到端链路。
-- ============================================================
-- （dek_he_ff 已在文件开头创建并持续存在，此处直接复用，不再重建）
SELECT '=== 25 multi-dek create ===';
DROP TABLE IF EXISTS t_multi CASCADE;
CREATE TABLE t_multi (
    id SERIAL,
    da  INT4_ENC ENCRYPTED WITH (DEK = 'dek_sym_he', HE_ADD_DEK = 'dek_he', SUPPORT_ALGO = 'det,he-add'),
    dm  INT4_ENC ENCRYPTED WITH (DEK = 'dek_sym_he', HE_MUL_DEK = 'dek_he_ff', SUPPORT_ALGO = 'det,he-mul'),
    dax INT4_ENC ENCRYPTED WITH (DEK = 'dek_sym_he', HE_ADD_DEK = 'dek_he', HE_MUL_DEK = 'dek_he_ff', SUPPORT_ALGO = 'det,he-add,he-mul'),
    oa  INT4_ENC ENCRYPTED WITH (DEK = 'dek_sym_he', HE_ADD_DEK = 'dek_he', SUPPORT_ALGO = 'ope,he-add'),
    sa  VARCHAR_ENC ENCRYPTED WITH (DEK = 'dek_sym_he', SUPPORT_ALGO = 'search')
);
INSERT INTO t_multi (da, dm, dax, oa, sa) VALUES (10, 20, 30, 100, 'multi-dek-search');
SELECT '=== 25 multi-dek roundtrip ===';
SELECT id, da, dm, dax, oa, sa FROM t_multi;
SELECT '=== 25 multi-dek HE ops ===';
SELECT id, da + da AS da_add, dm * dm AS dm_mul, dax + dax AS dax_add, dax * dax AS dax_mul FROM t_multi;
SELECT '=== 25 multi-dek symmetric predicates ===';
SELECT id, da FROM t_multi WHERE da = 10;
SELECT id, oa FROM t_multi WHERE oa BETWEEN 50 AND 150;
SELECT id, sa FROM t_multi WHERE sa LIKE '%search%';
SELECT '=== 25 multi-dek sum ===';
SELECT sum(da) FROM t_multi;

-- ============================================================
-- 12. 轮询 — 检查加密/解密仍正常
-- ============================================================
SELECT '=== 12 recheck ===';
SELECT count(*) AS cnt FROM he_test;

-- ============================================================
-- 24c. 幂运算值正确性与结果类型使用验证
-- ============================================================
-- a ^ 1 = a（幂等）
SELECT '=== 24c pow identity: i4_m ^ 1 ===';
SELECT id, i4_m ^ 1 AS p1 FROM he_test ORDER BY id;
SELECT '=== 24c pow identity: i2_m ^ 1 ===';
SELECT id, i2_m ^ 1 AS p1 FROM he_test ORDER BY id;
SELECT '=== 24c pow identity: i8_m ^ 1 ===';
SELECT id, i8_m ^ 1 AS p1 FROM he_test ORDER BY id;
-- NULL 输入
SELECT '=== 24c pow NULL input ===';
SELECT id, NULL ^ 2 AS pn FROM he_test ORDER BY id;
SELECT '=== 24c pow NULL exponent ===';
SELECT id, i4_m ^ NULL AS pne FROM he_test ORDER BY id;
-- 负底数奇次幂（验证符号正确）
SELECT '=== 24c pow negative base odd exp ===';
SELECT id, i4_m ^ 3 AS p3 FROM he_test ORDER BY id;

-- ============================================================
-- 24d. 幂运算错误路径：密文段不兼容应报错
-- ============================================================
-- pow 结果（EG_MUL）+ 常量（需要 EG_ADD）→ 应报错
SELECT '=== 24d pow ERR: mul result + he-add constant ===';
SELECT id, (i4_m ^ 2) + 10 AS pow_add FROM he_test ORDER BY id;

-- ============================================================
-- 98. Window function aggregate on HE column (should error)
-- ============================================================
SELECT '=== 98 WINDOW FUNCTION AGGREGATE ===';
-- SUM() OVER() on HE-ADD column: client must reject (no server-side
-- HE accumulation path in window aggregate executor).
SELECT '98a_sum_over', id, i4_a, SUM(i4_a) OVER (ORDER BY id) FROM he_test ORDER BY id;

-- ============================================================
-- 100. HE integer overflow range check (A10)
-- ============================================================
SELECT '=== 100 HE INTEGER OVERFLOW RANGE CHECK ===';

-- 100a: he-add int4 overflow (2147483647+1)
CREATE TABLE t_ovf_add_int4(a int4_enc ENCRYPTED WITH (HE_ADD_DEK=dek_he, SUPPORT_ALGO='he-add'));
INSERT INTO t_ovf_add_int4 VALUES (2147483647);
SELECT '100a_headd_int4_overflow', a+1 FROM t_ovf_add_int4;

-- 100b: he-add int8 overflow (9223372036854775807+1)
CREATE TABLE t_ovf_add_int8(a int8_enc ENCRYPTED WITH (HE_ADD_DEK=dek_he, SUPPORT_ALGO='he-add'));
INSERT INTO t_ovf_add_int8 VALUES (9223372036854775807);
SELECT '100b_headd_int8_overflow', a+1 FROM t_ovf_add_int8;

-- 100c: he-mul int4 overflow (50000*50000)
CREATE TABLE t_ovf_mul_int4(a int4_enc ENCRYPTED WITH (HE_MUL_DEK=dek_he_ff, SUPPORT_ALGO='he-mul'));
INSERT INTO t_ovf_mul_int4 VALUES (50000);
SELECT '100c_hemul_int4_overflow', a*50000 FROM t_ovf_mul_int4;

-- 100d: normal value (no overflow) - should succeed
SELECT '100d_normal_no_overflow', i4_a + 1 FROM he_test WHERE id = 1;

-- cleanup overflow tables
DROP TABLE t_ovf_add_int4, t_ovf_add_int8, t_ovf_mul_int4;

-- ============================================================
-- 26. HE SUM 视图查询（Bug 9150: sum(he-add) 视图查询报 DEK 解密失败）
--     视图聚合结果列无 col_meta 缓存时，HE-only 检查被跳过导致
--     对称 DEK 解密报错；应从密文段结构判断 HE-only 并跳过。
-- ============================================================
SELECT '=== 26 HE SUM view ===';
DROP VIEW IF EXISTS v_he_sum CASCADE;
CREATE VIEW v_he_sum AS SELECT sum(i4_a) AS s FROM he_test;
SELECT * FROM v_he_sum;
DROP VIEW v_he_sum CASCADE;

-- ============================================================
-- 99. Cleanup
-- ============================================================
SELECT '=== 99 CLEANUP ===';
DROP TABLE he_test CASCADE;
DROP TABLE t_assign CASCADE;
DROP TABLE IF EXISTS t_multi CASCADE;
DROP DEK IF EXISTS dek_he_ff CASCADE;
DROP TABLE IF EXISTS t_bad1;
DROP TABLE IF EXISTS t_bad2;
DROP TABLE IF EXISTS t_bad3;
DROP DEK IF EXISTS dek_bad_he CASCADE;
DROP DEK dek_sym_he;
DROP KEK kek_sym_he CASCADE;
DROP DEK dek_he;
DROP KEK kek_he CASCADE;
\! rm -f "$TECC_KEK_PATH/kek_he_asym.priv" "$TECC_KEK_PATH/kek_he_asym.pub" 2>/dev/null
\! rm -f "$TECC_KEK_PATH/kek_sym.key" 2>/dev/null
