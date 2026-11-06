-- 清理
DROP TABLE IF EXISTS test_text_indexes CASCADE;
DROP DEK IF EXISTS dek_t1 CASCADE;
DROP KEK IF EXISTS kek_t1 CASCADE;

-- 前置条件
CREATE KEK kek_t1 WITH (KEY_STORE=localkms, KEY_PATH='/tmp/kek_t1.key');
CREATE DEK dek_t1 WITH VALUES (KEK=kek_t1);

-- 测试步骤
create table test_text_indexes(id SERIAL PRIMARY KEY, text_col text_enc encrypted with (dek = dek_t1,support_algo='ope'),category VARCHAR(50),created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP);

INSERT INTO test_text_indexes (text_col, category) SELECT '这是测试文本数据', 'Category_' || (i % 10) FROM generate_series(1, 5000) i;

CREATE INDEX idx_text_btree ON test_text_indexes USING btree (text_col);

EXPLAIN (ANALYZE, BUFFERS) SELECT * FROM test_text_indexes WHERE text_col = '这是测试文本数据100';

SELECT indexname,indexdef,pg_size_pretty(pg_relation_size(indexname::regclass)) as index_size FROM pg_indexes WHERE tablename = 'test_text_indexes';

-- 清理
DROP TABLE IF EXISTS test_text_indexes CASCADE;
DROP DEK IF EXISTS dek_t1 CASCADE;
DROP KEK IF EXISTS kek_t1 CASCADE;
