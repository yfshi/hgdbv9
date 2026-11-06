-- 只测试INSERT语句，假设表已经存在
-- 1. 清理现有的测试对象
DROP TABLE IF EXISTS test_insert_only;

-- 2. 创建普通表格（没有加密字段）
CREATE TABLE test_insert_only (
    id SERIAL,
    name VARCHAR(50),
    salary INTEGER
);

-- 3. 插入数据
INSERT INTO test_insert_only (name, salary) VALUES ('Alice', 50000);

-- 4. 查询数据
SELECT * FROM test_insert_only;

-- 5. 清理
DROP TABLE test_insert_only;