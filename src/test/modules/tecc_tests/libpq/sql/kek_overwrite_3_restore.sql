--
-- kek_overwrite_3_restore.sql — 恢复原密钥文件后数据应可读
--   （证明覆盖后密文是密钥不匹配所致、数据未丢），最后清理。
--

-- ============================================================
-- 1. 恢复原密钥文件（新连接会重新加载原密钥）
-- ============================================================
SELECT '=== 1 restore original KEK file ===';

\! cp "$TECC_KEK_PATH/kek_ovr_orig.key" "$TECC_KEK_PATH/kek_ovr.key"

-- ============================================================
-- 2. SELECT：数据恢复可读（明文）
-- ============================================================
SELECT '=== 2 SELECT after restore (plaintext) ===';

SELECT id, name FROM ovr_test ORDER BY id;

-- ============================================================
-- 3. 清理
-- ============================================================
SELECT '=== 3 CLEANUP ===';

DROP TABLE ovr_test CASCADE;
DROP DEK dek_ovr;
DROP KEK kek_ovr CASCADE;
\! rm -f "$TECC_KEK_PATH/kek_ovr.key" "$TECC_KEK_PATH/kek_ovr_orig.key" 2>/dev/null
