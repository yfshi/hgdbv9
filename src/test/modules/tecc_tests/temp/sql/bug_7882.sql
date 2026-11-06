-- Bug 7882: OPE等值查询返回空（应报错）
-- 前置条件: KEK kek_t1 和 DEK dek_t1 已存在

DROP TABLE IF EXISTS test_ope_eq;
CREATE TABLE test_ope_eq(
    id SERIAL PRIMARY KEY,
    text_col text_enc encrypted with (dek = dek_t1, support_algo='ope'),
    category VARCHAR(50)
);

INSERT INTO test_ope_eq (text_col, category) VALUES ('这是测试文本数据', 'Category_A');
INSERT INTO test_ope_eq (text_col, category) VALUES ('hello world', 'Category_B');

-- 等值查询
SELECT * FROM test_ope_eq WHERE text_col = '这是测试文本数据';
