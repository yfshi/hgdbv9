--
-- kek_overwrite_2_verify.sql — 新连接（加载被覆盖的 KEK）：
--   SELECT 应返回密文（skipped_decrypt 回退，不报错）；
--   INSERT 需要 DEK 解密，应报错写不进去。
--

-- ============================================================
-- 1. SELECT：DEK 解密失败 → 返回 \x 密文（不报错）
-- ============================================================
SELECT '=== 1 SELECT after overwrite (expect ciphertext) ===';

SELECT id, name FROM ovr_test ORDER BY id;

-- ============================================================
-- 2. INSERT：加密需要 DEK → 解密失败报错
-- ============================================================
SELECT '=== 2 INSERT after overwrite (expect error) ===';

INSERT INTO ovr_test (name) VALUES ('newdata');
