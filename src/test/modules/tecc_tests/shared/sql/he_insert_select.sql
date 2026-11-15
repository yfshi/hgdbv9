-- ============================================================================
-- TECC INSERT ... SELECT 写入同态加密列
-- ============================================================================
-- 覆盖外场反馈缺陷：同态加密列上以"源为 SELECT 的裸常量"插入
--     INSERT INTO tb (id, a) SELECT 1, 2;
-- 会走 INSERT...SELECT 裸常量加密分支，该分支当时没有像 VALUES 路径那样把
-- 列上的 HE_ADD_DEK/HE_MUL_DEK 名交给调度器（纯 HE 列没有对称 DEK，
-- col_item->dek_name 为 NULL），调度器因此跳过 EC/FF 公钥加载，
-- HE_ADD/HE_MUL 加密失败。
--
-- 更严重的是失败路径对"调用方提供的列元数据"无条件调用
-- tecc_free_encrypted_column_metadata，而该元数据是
-- tecc_unified_encrypt_string_with_options 里的栈对象、其字符串字段还指向
-- 缓存私有内存，于是 free 栈地址 + double free，glibc 报
--     free(): invalid pointer
-- 直接 abort 客户端进程 —— 外场看到的现象就是"SQL 连接会断开"。
--
-- 本用例同时锁定三点：
--   1) INSERT...SELECT 同态列不再中断连接（psql 进程若 abort，
--      后续输出整体缺失，比对立即失败）；
--   2) 写入的密文有效 —— 回读明文正确，且能参与同态运算；
--   3) 混合列（det + he-add）两段密文都正确，DET 等值查询可命中。
-- ============================================================================

-- ### 环境准备 ###############################################################
-- HE DEK 必须挂在非对称 KEK（SM2）下，对称 KEK 会被拒绝。
-- 密钥生成到 $TECC_KEK_PATH（本回归独立目录，避免并发回归相互覆盖）；
-- CREATE KEK 的 key_path 不参与 TECC_KEK_PATH 覆盖，故用 %KEKPATH% 占位符。
\! rm -f "$TECC_KEK_PATH/kek_he_is_asym.priv" "$TECC_KEK_PATH/kek_he_is_asym.pub" 2>/dev/null
\! tecc_ktool generate --key-name kek_he_is_asym --key-algo sm2 --key-path "$TECC_KEK_PATH" >/dev/null 2>&1
\! rm -f "$TECC_KEK_PATH/kek_he_is_sym.key" 2>/dev/null
\! tecc_ktool generate --key-name kek_he_is_sym --key-algo sm4 --key-path "$TECC_KEK_PATH" >/dev/null 2>&1

DROP TABLE IF EXISTS he_is_test CASCADE;
DROP DEK IF EXISTS he_is_dek_sym CASCADE;
DROP DEK IF EXISTS he_is_dek_add CASCADE;
DROP DEK IF EXISTS he_is_dek_mul CASCADE;
DROP KEK IF EXISTS he_is_kek_asym CASCADE;
DROP KEK IF EXISTS he_is_kek_sym CASCADE;

CREATE KEK he_is_kek_asym WITH (
    key_store = 'localkms',
    key_algo = 'sm2',
    key_path = '%KEKPATH%/kek_he_is_asym.priv'
);
CREATE KEK he_is_kek_sym WITH (
    key_store = 'localkms',
    key_algo = 'sm4',
    key_path = '%KEKPATH%/kek_he_is_sym.key'
);
CREATE DEK he_is_dek_sym WITH VALUES(KEK = he_is_kek_sym);
CREATE DEK he_is_dek_add WITH VALUES(KEK = he_is_kek_asym, KEY_ALGO = 'he-add');
CREATE DEK he_is_dek_mul WITH VALUES(KEK = he_is_kek_asym, KEY_ALGO = 'he-mul');

CREATE TABLE he_is_test (
    id int,
    a  int4_enc ENCRYPTED WITH (HE_ADD_DEK = 'he_is_dek_add', SUPPORT_ALGO = 'he-add'),
    m  int8_enc ENCRYPTED WITH (HE_MUL_DEK = 'he_is_dek_mul', SUPPORT_ALGO = 'he-mul'),
    x  int4_enc ENCRYPTED WITH (DEK = 'he_is_dek_sym', HE_ADD_DEK = 'he_is_dek_add', SUPPORT_ALGO = 'det,he-add'),
    i2 int2_enc ENCRYPTED WITH (HE_ADD_DEK = 'he_is_dek_add', SUPPORT_ALGO = 'he-add')
);

-- ### 1. 缺陷原形：裸常量 INSERT ... SELECT ####################################
\echo '========== 1. INSERT ... SELECT 裸常量写入同态列（缺陷原形） =========='
INSERT INTO he_is_test (id, a) SELECT 1, 2;
SELECT 'IS-1', id, a FROM he_is_test WHERE id = 1;

-- ### 2. 多列 / 省略列清单 / 列清单换序 ########################################
\echo '========== 2. 多列、省略列清单、列清单换序 =========='
INSERT INTO he_is_test (id, a, m, x, i2) SELECT 2, 10, 3, 13, 100;
SELECT 'IS-2-cols', id, a, m, x, i2 FROM he_is_test WHERE id = 2;

INSERT INTO he_is_test SELECT 3, 20, 4, 23, 200;
SELECT 'IS-2-nolist', id, a, m, x, i2 FROM he_is_test WHERE id = 3;

INSERT INTO he_is_test (a, id) SELECT 50, 4;
SELECT 'IS-2-reorder', id, a FROM he_is_test WHERE id = 4;

-- ### 3. 密文有效性：回读 + 同态运算 ##########################################
\echo '========== 3. 回读明文与同态运算 =========='
SELECT 'IS-3-read', id, a, m, x, i2 FROM he_is_test ORDER BY id;
-- he-add 列参与同态加减
SELECT 'IS-3-add', a, a + 5, a - 1 FROM he_is_test WHERE id = 1;
SELECT 'IS-3-i2-add', i2, i2 + 1 FROM he_is_test WHERE id = 2;
-- he-mul 列参与同态标量乘
SELECT 'IS-3-mul', m, m * 4 FROM he_is_test WHERE id = 2;
-- 混合列 det+he-add：DET 段可等值命中，HE 段可参与同态运算
SELECT 'IS-3-mix-det', count(*) FROM he_is_test WHERE x = 13;
SELECT 'IS-3-mix-he', x, x + 1 FROM he_is_test WHERE id = 2;

-- ### 4. VALUES 与 SELECT 两条路径密文互操作 ##################################
\echo '========== 4. VALUES 与 SELECT 密文互操作 =========='
INSERT INTO he_is_test (id, a) VALUES (5, 30);
INSERT INTO he_is_test (id, a) SELECT 6, 40;
-- 两条路径分别写入 1(2) / 5(30) / 6(40)，同态求和须一次算对；
-- 这是对"两条路径产出的密文格式一致、可互相参与同态运算"的直接验证。
SELECT 'IS-4-sum', sum(a) FROM he_is_test WHERE id IN (1, 5, 6);

-- ### 5. 清理 ################################################################
\echo '========== 5. 清理 =========='
SELECT 'IS-END';
DROP TABLE he_is_test CASCADE;
DROP DEK he_is_dek_sym CASCADE;
DROP DEK he_is_dek_add CASCADE;
DROP DEK he_is_dek_mul CASCADE;
DROP KEK he_is_kek_asym CASCADE;
DROP KEK he_is_kek_sym CASCADE;
