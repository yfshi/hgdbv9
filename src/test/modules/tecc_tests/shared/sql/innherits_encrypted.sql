-- ============================================================
-- inherits_encrypted.sql — INHERITS 继承密态列报错回归测试
--
-- 背景（08-30 文档 #6）：
--   CREATE TABLE child (...) INHERITS (parent) 在 parent 含
--   加密列时，MergeAttributes 丢弃 encrypted_with，导致即使
--   子表显式重定义 ENCRYPTED WITH 也报错。
--   修复：给出明确的不支持报错，而非误导性的
--   "requires ENCRYPTED WITH clause"。
-- ============================================================

-- ============================================================
-- 0. Setup
-- ============================================================
DROP KEK IF EXISTS inh_kek CASCADE;
DROP DEK IF EXISTS inh_dek CASCADE;

CREATE KEK inh_kek WITH (key_store='localkms', key_path='/tmp/inh_kek.key');
CREATE DEK inh_dek WITH VALUES(KEK=inh_kek);

-- ============================================================
-- 1. INHERITS + 加密列 → 明确报不支持
-- ============================================================
CREATE TABLE inh_parent (id int4, c_det int4_enc ENCRYPTED WITH (DEK=inh_dek, SUPPORT_ALGO='det'));
CREATE TABLE inh_child (c_extra text) INHERITS (inh_parent);

-- ============================================================
-- 2. 清理
-- ============================================================
DROP TABLE IF EXISTS inh_parent CASCADE;
DROP DEK IF EXISTS inh_dek CASCADE;
DROP KEK IF EXISTS inh_kek CASCADE;
