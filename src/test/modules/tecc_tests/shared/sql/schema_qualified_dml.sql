--
-- schema_qualified_dml.sql — 限定 schema 的 DML 字面量加密回归测试
--
-- 回归场景（修复提交 b95a0199ab）：
--   1. 在 sch1 下建加密表 sch1.tb1（age numeric_enc），插入/更新正常
--   2. 在 public 下建同名表 tb1（关键：age 为明文 numeric）
--   3. 再对 sch1.tb1 执行限定表名的 UPDATE/INSERT/WHERE/DELETE
--
-- 修复前 bug：限定表名的 DML 字面量加密，元数据查询用 NULL/current_schema
-- （search_path 首个模式 = public）查找，search_path 不含 sch1 时漏检到
-- public.tb1（age 明文）→ 字面量未加密 → 后端类型错误
--   "column \"age\" is of type numeric_enc but expression is of type numeric"。
-- 修复后：限定表名按 relation->schemaname 解析列元数据，字面量正常加密。
-- 未限定表名仍走 search_path（此处解析到 public.tb1 明文表），不受影响。
-- ============================================================

-- ============================================================
-- 0. Setup
-- ============================================================
DROP SCHEMA IF EXISTS sch1 CASCADE;
DROP KEK IF EXISTS kek_test CASCADE;
DROP DEK IF EXISTS dek_test CASCADE;

CREATE KEK kek_test WITH (key_store='localkms', key_path='/tmp/kek_test.key');
CREATE DEK dek_test WITH VALUES(KEK=kek_test);

CREATE SCHEMA sch1;
CREATE TABLE sch1.tb1 (id int, age numeric_enc ENCRYPTED WITH (DEK='dek_test', support_algo='det'));
-- public 同名表：age 明文，用于捕获"限定表名错误解析到 public"的回归
CREATE TABLE tb1 (id int, age numeric);

INSERT INTO sch1.tb1 VALUES (1, 22.333);

-- ============================================================
-- 1. 限定 schema 的 UPDATE SET 字面量加密（核心回归断言）
-- ============================================================
SELECT '=== 1 qualified UPDATE SET ===';
UPDATE sch1.tb1 SET age = 33.444 WHERE id = 1;
SELECT id, age FROM sch1.tb1 ORDER BY id;
-- public 同名明文表不受影响（仍 0 行）
SELECT id, age FROM tb1 ORDER BY id;

-- ============================================================
-- 2. 限定 schema 的 WHERE 字面量加密（加密列 = 字面量）
-- ============================================================
SELECT '=== 2 qualified WHERE literal ===';
UPDATE sch1.tb1 SET age = 44.555 WHERE age = 33.444;
SELECT id, age FROM sch1.tb1 ORDER BY id;
-- WHERE 命中 0 行（33.444 已被 44.555 覆盖，不应再更新）
UPDATE sch1.tb1 SET age = 99.999 WHERE age = 33.444;
SELECT id, age FROM sch1.tb1 ORDER BY id;

-- ============================================================
-- 3. 限定 schema 的 INSERT 字面量加密
-- ============================================================
SELECT '=== 3 qualified INSERT ===';
INSERT INTO sch1.tb1 VALUES (2, 55.666);
SELECT id, age FROM sch1.tb1 ORDER BY id;

-- ============================================================
-- 4. 限定 schema 的 DELETE（WHERE 走 where_clause_processor 的 schema 传递）
-- ============================================================
SELECT '=== 4 qualified DELETE ===';
DELETE FROM sch1.tb1 WHERE id = 2;
SELECT id, age FROM sch1.tb1 ORDER BY id;

-- ============================================================
-- 5. 未限定表名仍走 search_path（解析到 public.tb1 明文表）
-- ============================================================
SELECT '=== 5 unqualified resolves via search_path ===';
INSERT INTO tb1 VALUES (3, 9.99);
UPDATE tb1 SET age = 77.888 WHERE id = 3;
SELECT id, age FROM tb1 ORDER BY id;
-- sch1.tb1 不受未限定 DML 影响
SELECT id, age FROM sch1.tb1 ORDER BY id;

-- ============================================================
-- 9. 清理
-- ============================================================
SELECT '=== 9 CLEANUP ===';
DROP TABLE sch1.tb1, tb1 CASCADE;
DROP SCHEMA sch1;
DROP DEK dek_test;
DROP KEK kek_test CASCADE;
