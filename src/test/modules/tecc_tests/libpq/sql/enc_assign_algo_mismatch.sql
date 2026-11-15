-- ============================================================
-- enc_assign_algo_mismatch.sql — 跨算法赋值必须在写入点被拒绝
--
-- 背景（QA 报修 Bug 9313 / 9314）：
--   1) Bug 9313：`UPDATE t SET o = d`（o 声明 OPE、d 声明 DET，两列同一把 DEK）
--      执行成功，只带 DET 段的密文被写进了 OPE 列；写回时"看似正常"（能解出
--      明文），直到等值/范围查询才报
--      "neither DET nor OPE ciphertext available" / "OPE ciphertext not available"。
--   2) Bug 9314：`UPDATE t SET h = d`（h 声明 he-add、d 声明 DET，同一把 DEK）
--      执行成功，对称密文被写进纯 he-add 列；SELECT 直接把原始密文返回给客户端，
--      同态运算报 "operand lacks EG_ADD ciphertext"。
--
-- 根因：写入点（tecc_validate_encrypted_slots）只校验密文自带的 DEK 身份是否等于
--   目标列配置的 DEK，没有校验密文【携带的算法段】是否覆盖目标列 support_algo
--   声明的算法；缺失 HE 段原先只发 WARNING、不阻断写入，对称/搜索段则完全无人
--   校验。前端对 `SET 列 = 列` 这类形状一律透传密文（stmt_processor.c 的
--   "passing through for server-side copy"），于是同 DEK 跨算法赋值两头都没人管，
--   列声明的算法能力与实际数据背离。
--
-- 修复：新增 tecc_check_cipher_segments —— 写入前要求密文携带目标列声明的每一个
--   算法段（DET/OPE/SEARCH/he-add/he-mul），缺失即 ERRCODE_TECC_ALGORITHM_INCOMPATIBLE
--   硬报错（缺失 HE 段由 WARNING 升级为同样的报错）。浮点哨兵值（Infinity）没有
--   真实密文段，跳过；DEK 身份校验保持原样。
--
-- 判定点：
--   第 2 节四个报修/镜像形态（同 DEK 跨算法赋值）必须全部被拒，且被拒后列值、
--   本列算法能力（OPE 范围、DET 等值）保持完好；
--   第 3 节同算法赋值、常量写入、同态结果回写纯 HE 列必须照常成功——修复只拦
--   "跨算法"，不得扩大到正常写入。
--
-- 归属：赋值发生在服务端执行器写入点，与驱动无关；但触发形状依赖前端"列=列
--   原样透传"，故放 libpq/sql。若日后前端也拦截这些形状，本用例的期望会从服务端
--   ERROR 变成前端 "TECC query preprocessing failed: ..."，届时按新行为更新即可。
-- ============================================================

\set ON_ERROR_STOP off

-- ============================================================
-- 0. 幂等清理 + 建密钥：一把对称 KEK（四个列共用一把 DEK）、一把 SM2 KEK 派生
--    两个同态 DEK（he-add / he-mul）
-- ============================================================
DROP TABLE IF EXISTS eam_t, eam_ref, eam_he CASCADE;
DROP DEK IF EXISTS eam_d CASCADE;
DROP DEK IF EXISTS eam_add CASCADE;
DROP DEK IF EXISTS eam_mul CASCADE;
DROP KEK IF EXISTS eam_ksym CASCADE;
DROP KEK IF EXISTS eam_khe CASCADE;

\! rm -f "$TECC_KEK_PATH/eam_ksym.key" 2>/dev/null
\! rm -f "$TECC_KEK_PATH/eam_khe.priv" "$TECC_KEK_PATH/eam_khe.pub" 2>/dev/null
\! tecc_ktool generate --key-name eam_ksym --key-algo sm4 --key-path "$TECC_KEK_PATH" >/dev/null 2>&1
\! tecc_ktool generate --key-name eam_khe  --key-algo sm2 --key-path "$TECC_KEK_PATH" >/dev/null 2>&1

CREATE KEK eam_ksym WITH (
    key_store = 'localkms',
    key_algo  = 'sm4',
    key_path  = '%KEKPATH%/eam_ksym.key'
);
CREATE KEK eam_khe WITH (
    key_store = 'localkms',
    key_algo  = 'sm2',
    key_path  = '%KEKPATH%/eam_khe.priv'
);
CREATE DEK eam_d   WITH VALUES(KEK = eam_ksym);
CREATE DEK eam_add WITH VALUES(KEK = eam_khe, KEY_ALGO = 'he-add');
CREATE DEK eam_mul WITH VALUES(KEK = eam_khe, KEY_ALGO = 'he-mul');

-- ============================================================
-- 1. 目标表：四个列共用同一把对称 DEK，算法各不相同
--    （HE 列另绑各自的同态 DEK，即多 DEK 模式）
-- ============================================================
SELECT '=== 1 建表 ===' AS stage;
CREATE TABLE eam_t (
    det  int4_enc ENCRYPTED WITH (DEK = 'eam_d', SUPPORT_ALGO = 'det'),
    ope  int4_enc ENCRYPTED WITH (DEK = 'eam_d', SUPPORT_ALGO = 'ope'),
    addc int4_enc ENCRYPTED WITH (DEK = 'eam_d', HE_ADD_DEK = 'eam_add',
                                  SUPPORT_ALGO = 'he-add'),
    mulc int4_enc ENCRYPTED WITH (DEK = 'eam_d', HE_MUL_DEK = 'eam_mul',
                                  SUPPORT_ALGO = 'he-mul')
);
INSERT INTO eam_t (det, ope) VALUES (3, 30);

-- ============================================================
-- 2. 报修形态与镜像：同 DEK 跨算法赋值必须被写入点拒绝
-- ============================================================
SELECT '=== 2.1 Bug 9313: DET 密文 → OPE 列 ===' AS stage;
UPDATE eam_t SET ope = det;

SELECT '=== 2.2 Bug 9313 镜像: OPE 密文 → DET 列 ===' AS stage;
UPDATE eam_t SET det = ope;

SELECT '=== 2.3 Bug 9314: 对称密文 → he-add 列 ===' AS stage;
UPDATE eam_t SET addc = det;

SELECT '=== 2.4 Bug 9314 镜像: 对称密文 → he-mul 列 ===' AS stage;
UPDATE eam_t SET mulc = ope;

SELECT '=== 2.5 被拒后列值与本列算法能力保持完好 ===' AS stage;
SELECT det, ope, addc, mulc FROM eam_t;
SELECT count(*) AS ope_range_ok FROM eam_t WHERE ope > 25;
SELECT count(*) AS det_eq_ok    FROM eam_t WHERE det = 3;

-- ============================================================
-- 3. 不得误杀：同算法赋值、常量写入、同态结果回写必须照常成功
-- ============================================================
SELECT '=== 3.1 同算法跨表赋值（子查询 / FROM） ===' AS stage;
CREATE TABLE eam_ref (
    det int4_enc ENCRYPTED WITH (DEK = 'eam_d', SUPPORT_ALGO = 'det'),
    ope int4_enc ENCRYPTED WITH (DEK = 'eam_d', SUPPORT_ALGO = 'ope')
);
INSERT INTO eam_ref (det, ope) VALUES (0, 0);
UPDATE eam_ref SET ope = (SELECT ope FROM eam_t LIMIT 1);
UPDATE eam_ref SET det = t.det FROM eam_t t;
SELECT det, ope FROM eam_ref;
SELECT count(*) AS ref_range_ok FROM eam_ref WHERE ope > 25;

SELECT '=== 3.2 常量写入 ===' AS stage;
UPDATE eam_t SET det = 3, ope = 30;
INSERT INTO eam_t (det, ope) VALUES (4, 40);
SELECT count(*) AS ope_eq_ok FROM eam_t WHERE ope = 30;

SELECT '=== 3.3 同态结果回写纯 HE 列 ===' AS stage;
CREATE TABLE eam_he (
    s int4_enc ENCRYPTED WITH (DEK = 'eam_d', HE_ADD_DEK = 'eam_add',
                               SUPPORT_ALGO = 'he-add'),
    m int4_enc ENCRYPTED WITH (DEK = 'eam_d', HE_MUL_DEK = 'eam_mul',
                               SUPPORT_ALGO = 'he-mul')
);
INSERT INTO eam_he (s, m) VALUES (7, 5);
UPDATE eam_he SET s = s + s;
UPDATE eam_he SET m = m * 2;
SELECT sum(s) AS sum_s FROM eam_he;
SELECT s, m FROM eam_he;

-- ============================================================
-- 4. 清理
-- ============================================================
DROP TABLE eam_he;
DROP TABLE eam_ref;
DROP TABLE eam_t;
DROP DEK eam_d;
DROP DEK eam_add;
DROP DEK eam_mul;
DROP KEK eam_ksym;
DROP KEK eam_khe CASCADE;
