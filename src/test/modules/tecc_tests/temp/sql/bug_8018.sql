--
-- bug_8018.sql — G9 加密列建索引检查
-- 不指定ope/det时(RND-only)，btree和hash索引都应报错
--
-- 根因: get_encrypted_column_encryption_type()返回0无法区分"非加密列"和
--       "RND-only加密列"，check_index_support_for_encrypted_column全部放行
-- 修复: tecc_index.c中函数改为返回bool+出参，区分两种情况
--

DROP TABLE IF EXISTS test_idx_8018 CASCADE;
DROP DEK IF EXISTS dek_g9 CASCADE;
DROP KEK IF EXISTS kek_g9 CASCADE;

CREATE KEK kek_g9 WITH (key_store = 'localkms', key_path = '/tmp/kek_g9.key');
CREATE DEK dek_g9 WITH VALUES(KEK = 'kek_g9');

CREATE TABLE test_idx_8018 (
    id SERIAL PRIMARY KEY,
    text_col text_enc ENCRYPTED WITH (dek = dek_g9)
);

-- 以下两个CREATE INDEX都应报错
CREATE INDEX idx_btree ON test_idx_8018 USING btree (text_col);
CREATE INDEX idx_hash ON test_idx_8018 USING hash (text_col);

DROP TABLE test_idx_8018 CASCADE;
DROP DEK dek_g9 CASCADE;
DROP KEK kek_g9 CASCADE;
