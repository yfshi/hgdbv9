-- ============================================================
-- swxa_alter_kek.sql — SWXA（三未信安）KEK 轮换（ALTER KEK）测试
-- ============================================================
-- 覆盖: 密码机 KEK 从 hsm:id=1 轮换到 hsm:id=2
--       DEK 用旧 KEK 解密、新 KEK 重加密
--       轮换后：旧数据可解密、等值/范围查询正常、新数据可写入
-- ============================================================

-- 清理
DROP TABLE IF EXISTS swxa_rot_t CASCADE;
DROP DEK IF EXISTS swxa_rot_dek CASCADE;
DROP KEK IF EXISTS swxa_rot_kek CASCADE;

-- 1. 创建 SWXA KEK（预置 kek-1.key，hsm:id=1）
CREATE KEK swxa_rot_kek WITH (
    key_store = 'swxakms',
    key_path = 'hsm:id=1'
);

-- 2. 创建 DEK（对称 DEK 绑 HSM KEK，DEK 密文由密码机 SM4 加密）
CREATE DEK swxa_rot_dek WITH VALUES(KEK = swxa_rot_kek);

-- 3. 建加密表（DET 等值 + OPE 范围）
CREATE TABLE swxa_rot_t (
    id  SERIAL,
    c   INT4_ENC ENCRYPTED WITH (DEK = swxa_rot_dek, SUPPORT_ALGO = 'DET'),
    s   INT4_ENC ENCRYPTED WITH (DEK = swxa_rot_dek, SUPPORT_ALGO = 'OPE')
);

-- 4. 轮换前写入
INSERT INTO swxa_rot_t (c, s) VALUES (100, 10), (200, 20), (300, 30);

SELECT 'before', id, c, s FROM swxa_rot_t ORDER BY id;

-- 5. ALTER KEK 轮换到 hsm:id=2（预置 kek-2.key）
ALTER KEK swxa_rot_kek WITH (KEY_PATH = 'hsm:id=2');

-- 6. 轮换后旧数据解密（DEK 已用新 KEK 重加密）
SELECT 'after', id, c, s FROM swxa_rot_t ORDER BY id;

-- 7. 轮换后等值查询（DET）
SELECT 'det', id FROM swxa_rot_t WHERE c = 200;

-- 8. 轮换后范围查询（OPE）
SELECT 'ope', id FROM swxa_rot_t WHERE s >= 20 ORDER BY s;

-- 9. 轮换后插入新数据（用新 KEK 解密 DEK 后加密）
INSERT INTO swxa_rot_t (c, s) VALUES (400, 40), (500, 50);

-- 10. 全部数据解密验证（含新数据）
SELECT 'all', id, c, s FROM swxa_rot_t ORDER BY id;

-- 清理
DROP TABLE swxa_rot_t CASCADE;
DROP KEK swxa_rot_kek CASCADE;
