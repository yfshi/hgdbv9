-- Bug 8027: text_det(det,search) LIKE '张%' 中文前缀漏数据
-- Bug 8029: varchar_enc 的 ope+search 列无法 LIKE
-- Bug 8022: LIKE 查询会话异常断开
-- Bug 7998: LIKE+ORDER BY 返回多条数据报错

DROP KEK IF EXISTS kek_bug CASCADE;
CREATE KEK kek_bug WITH (key_store = 'localkms', key_path = '/tmp/kek_bug.key');
CREATE DEK dek_bug WITH VALUES(KEK = kek_bug);

DROP TABLE IF EXISTS enc CASCADE;
CREATE TABLE enc (
    id serial PRIMARY KEY,
    -- det+search: Bug 8027 scenario
    c_text_det text_enc ENCRYPTED WITH (dek = dek_bug, support_algo = 'det,search'),
    -- ope+search: Bug 8029 scenario
    c_text_ope text_enc ENCRYPTED WITH (dek = dek_bug, support_algo = 'ope,search'),
    c_varchar_det varchar_enc(100) ENCRYPTED WITH (dek = dek_bug, support_algo = 'det,search'),
    c_varchar_ope varchar_enc(100) ENCRYPTED WITH (dek = dek_bug, support_algo = 'ope,search'),
    c_bpchar_ope bpchar_enc(20) ENCRYPTED WITH (dek = dek_bug, support_algo = 'ope,search')
);

INSERT INTO enc (c_text_det, c_text_ope, c_varchar_det, c_varchar_ope, c_bpchar_ope) VALUES
('张三', '张三', '李四', '李四', '王五'),
('Alice', 'Alice', 'Bob', 'Bob', 'Charlie'),
('赵六', '赵六', '孙七', '孙七', '周八'),
('张三丰', '张三丰', 'David', 'David', 'Eve'),
('hello', 'hello', 'world', 'world', 'test'),
('alice', 'alice', 'ALICE', 'ALICE', 'alice'),
('张三丰', '张三丰', 'David2', 'David2', 'Eve2'),
('Hello', 'Hello', 'World', 'World', 'TEST'),
('abc_def', 'abc_def', 'xyz_123', 'xyz_123', 'foo_bar'),
('test%like', 'test%like', 'test_like','test_like','pct_sign'),
('%percent', '%percent', 'under_score','under_score','_start'),
(NULL, NULL, NULL, NULL, NULL),
('', '', '', '', ''),
('测试', '测试', '测试', '测试', '测试'),
('test123', 'test123', 'abcXYZ', 'abcXYZ', 'mixCase');

-- Bug 8027: 中文前缀 LIKE
SELECT 'bug8027_zhang', id, c_text_det FROM enc WHERE c_text_det LIKE '张%' ORDER BY id;

-- Bug 8029: varchar_enc ope+search LIKE
SELECT 'bug8029_varchar', id, c_varchar_ope FROM enc WHERE c_varchar_ope LIKE 'test%' ORDER BY id;

-- Bug 8022: bpchar_ope LIKE (session crash)
SELECT 'bug8022_bpchar', id, c_bpchar_ope FROM enc WHERE c_bpchar_ope LIKE '%test%';

-- Bug 7998: LIKE + ORDER BY multiple rows
SELECT 'bug7998_multi', id, c_text_det FROM enc WHERE c_text_det LIKE '%e%' ORDER BY id;

-- Additional edge cases
SELECT 'edge_case1', id, c_text_ope FROM enc WHERE c_text_ope LIKE '%张%' ORDER BY id;
SELECT 'edge_case2', id, c_varchar_det FROM enc WHERE c_varchar_det LIKE 'D%' ORDER BY id;
SELECT 'edge_case3', id, c_text_det FROM enc WHERE c_text_det LIKE '%a%' ORDER BY id;
SELECT 'edge_case4', id, c_bpchar_ope FROM enc WHERE c_bpchar_ope LIKE 'a%' ORDER BY id;
SELECT 'edge_case5', id, c_text_det FROM enc WHERE c_text_det LIKE '%123' ORDER BY id;

DROP TABLE IF EXISTS enc CASCADE;
DROP KEK IF EXISTS kek_bug CASCADE;
