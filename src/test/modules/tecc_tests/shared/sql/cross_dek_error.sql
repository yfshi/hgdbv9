-- ============================================================
-- cross_dek_error.sql
-- 跨 DEK 比较报错专项用例：不同 DEK 的加密列禁止任何比较（前端/服务端拦截报错）
--
-- 覆盖场景：
--   1. DET 等值跨 DEK：JOIN（INNER/LEFT/RIGHT/FULL）、WHERE 两列、IN/=ANY/=ALL/NOT IN/EXISTS 子查询
--   2. OPE 范围跨 DEK：范围 JOIN、WHERE 范围
--   3. LIKE 列对列：密态列与密态列之间不支持 LIKE/NOT LIKE（同 DEK 与跨 DEK 均报错）
--   各节均含同 DEK 对照用例（正常执行）
-- ============================================================

-- ============================================================
-- 0. Setup
-- ============================================================
DROP KEK IF EXISTS cdk_kek_b CASCADE;
DROP KEK IF EXISTS cdk_kek_c CASCADE;
DROP DEK IF EXISTS cdk_dek_b CASCADE;
DROP DEK IF EXISTS cdk_dek_c CASCADE;
CREATE KEK cdk_kek_b WITH (key_store='localkms', key_path='/tmp/cdk_kek_b.key');
CREATE KEK cdk_kek_c WITH (key_store='localkms', key_path='/tmp/cdk_kek_c.key');
CREATE DEK cdk_dek_b WITH VALUES(KEK=cdk_kek_b);
CREATE DEK cdk_dek_c WITH VALUES(KEK=cdk_kek_c);

DROP TABLE IF EXISTS cd_t_a, cd_t_b, cd_t_s1, cd_t_s2 CASCADE;
-- cd_t_a: v 为 DET 列（dek_b），s 为 OPE 列（dek_b）
CREATE TABLE cd_t_a(id serial, v int4_enc ENCRYPTED WITH (DEK='cdk_dek_b', SUPPORT_ALGO='det'),
                    s int4_enc ENCRYPTED WITH (DEK='cdk_dek_b', SUPPORT_ALGO='ope'));
-- cd_t_b: v 为 DET 列（dek_c），s 为 OPE 列（dek_c）——跨 DEK 对照表
CREATE TABLE cd_t_b(id serial, v int4_enc ENCRYPTED WITH (DEK='cdk_dek_c', SUPPORT_ALGO='det'),
                    s int4_enc ENCRYPTED WITH (DEK='cdk_dek_c', SUPPORT_ALGO='ope'));
-- LIKE 场景：SEARCH 列（同 DEK / 跨 DEK）
CREATE TABLE cd_t_s1(id serial, name varchar_enc ENCRYPTED WITH (DEK='cdk_dek_b', SUPPORT_ALGO='search'));
CREATE TABLE cd_t_s2(id serial, name varchar_enc ENCRYPTED WITH (DEK='cdk_dek_c', SUPPORT_ALGO='search'));

INSERT INTO cd_t_a(v,s) VALUES (1,10),(2,20),(3,30),(5,50);
INSERT INTO cd_t_b(v,s) VALUES (3,30),(4,40);
INSERT INTO cd_t_s1(name) VALUES ('Alice'),('Bob');
INSERT INTO cd_t_s2(name) VALUES ('Alice'),('Bob');

-- ============================================================
-- 1. DET 等值跨 DEK（均报错）
-- ============================================================
SELECT '=== 1 DET eq cross DEK ===';
-- 1a. INNER JOIN 等值 → 报错
SELECT '1a_inner_join', count(*) FROM cd_t_a a JOIN cd_t_b b ON a.v=b.v;
-- 1b. LEFT JOIN 等值 → 报错
SELECT '1b_left_join', count(*) FROM cd_t_a a LEFT JOIN cd_t_b b ON a.v=b.v;
-- 1c. RIGHT JOIN 等值 → 报错
SELECT '1c_right_join', count(*) FROM cd_t_a a RIGHT JOIN cd_t_b b ON a.v=b.v;
-- 1d. FULL JOIN 等值 → 报错
SELECT '1d_full_join', count(*) FROM cd_t_a a FULL JOIN cd_t_b b ON a.v=b.v;
-- 1e. WHERE 两列等值（逗号连接）→ 报错
SELECT '1e_where_eq', count(*) FROM cd_t_a a, cd_t_b b WHERE a.v=b.v;
-- 1f. IN 子查询 → 报错
SELECT '1f_in_sub', count(*) FROM cd_t_a a WHERE a.v IN (SELECT b.v FROM cd_t_b b);
-- 1g. =ANY 子查询 → 报错
SELECT '1g_any_sub', count(*) FROM cd_t_a a WHERE a.v = ANY (SELECT b.v FROM cd_t_b b);
-- 1h. =ALL 子查询 → 报错
SELECT '1h_all_sub', count(*) FROM cd_t_a a WHERE a.v = ALL (SELECT b.v FROM cd_t_b b);
-- 1i. NOT IN 子查询 → 报错
SELECT '1i_not_in', count(*) FROM cd_t_a a WHERE a.v NOT IN (SELECT b.v FROM cd_t_b b);
-- 1j. EXISTS 子查询 → 报错
SELECT '1j_exists', count(*) FROM cd_t_a a WHERE EXISTS (SELECT 1 FROM cd_t_b b WHERE b.v=a.v);
-- 1k. 同 DEK 跨表等值 JOIN（对照，正常）
SELECT '1k_same_dek', count(*) FROM cd_t_a a JOIN cd_t_a x ON a.v=x.v;

-- ============================================================
-- 2. OPE 范围跨 DEK（均报错）
-- ============================================================
SELECT '=== 2 OPE range cross DEK ===';
-- 2a. 范围 JOIN → 报错
SELECT '2a_range_join', count(*) FROM cd_t_a a JOIN cd_t_b b ON a.s < b.s;
-- 2b. WHERE 范围 → 报错
SELECT '2b_where_range', count(*) FROM cd_t_a a, cd_t_b b WHERE a.s < b.s;
-- 2c. 同 DEK 范围 JOIN（对照，正常）
SELECT '2c_same_range', count(*) FROM cd_t_a a JOIN cd_t_a x ON a.s < x.s;

-- ============================================================
-- 3. LIKE 列对列（密态列之间不支持 LIKE，均报错）
-- ============================================================
SELECT '=== 3 LIKE col-to-col ===';
-- 3a. 同 DEK LIKE 列对列 → 报错（不支持）
SELECT '3a_like_same_dek', count(*) FROM cd_t_s1 a JOIN cd_t_s1 x ON a.name LIKE x.name;
-- 3b. 跨 DEK LIKE 列对列 → 报错（不支持）
SELECT '3b_like_cross_dek', count(*) FROM cd_t_s1 a JOIN cd_t_s2 b ON a.name LIKE b.name;
-- 3c. LIKE 列 vs 常量（正常场景，对照）
SELECT '3c_like_const', count(*) FROM cd_t_s1 a WHERE a.name LIKE 'A%';

-- ============================================================
-- 9. CLEANUP
-- ============================================================
SELECT '=== 9 CLEANUP ===';
DROP TABLE cd_t_a, cd_t_b, cd_t_s1, cd_t_s2 CASCADE;
DROP DEK cdk_dek_b CASCADE;
DROP DEK cdk_dek_c CASCADE;
DROP KEK cdk_kek_b CASCADE;
DROP KEK cdk_kek_c CASCADE;
