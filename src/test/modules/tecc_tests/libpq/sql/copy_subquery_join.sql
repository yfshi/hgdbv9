-- ============================================================
-- copy_subquery_join.sql — \copy (SELECT ... JOIN ...) 子查询导出的密态列解密
--
-- 归属 libpq/：本用例打的是 libpq（C 客户端）COPY OUT 出口的解密实现
-- （pqGetCopyData3 → tecc_process_copy_result 按列元数据逐列解密），
-- 与 psql 元命令 + 客户端加密列元数据/密钥链路强绑定；JDBC 侧是另一套
-- COPY 实现（TeccSqlTestRunner），不在本用例覆盖范围内。
--
-- Bug 907：子查询 FROM 首项是 JOIN（JoinExpr）而不是单表 RangeVar 时，
-- "取第一张表名"的 fallback 落空 —— fe-tecc-copy.c 取不到 table_name、
-- stmt_processor.c 建不出列映射 —— 加密列元数据一条也加载不到，导出结果
-- 整行输出密文（连设计文档承诺的"第一张表解密"都没做到）。
-- 期望（用户裁决）：子查询结果里所有密态列都解密输出明文。
--
-- 本用例覆盖：
--   1. 单表子查询（既有能力，对照）
--   2. 显式 JOIN：两列分别来自左右表，都应明文（原缺陷场景）
--   3. 逗号连接：FROM 首项是真 RangeVar，修复前"碰巧"能解（对照）
--   4. 镜像：FROM 首项换成另一张表
--   5. 同一张表的两种密态列（靠列名/attnum 双查）
--   6. attnum 冲突 + 不同 DEK/算法：两表第 3 列都是密态列，但一列 DET 一列
--      OPE 且 DEK 不同 —— 只按 attnum 匹配会拿错表的元数据解密而失败
--   7. LEFT JOIN（同一 JoinExpr 递归路径）
--   8. 自连接：同一张表出现两次，别名不同
--   9. TEXT 格式
--  10. 三段限定列名（schema.表.列）
--  11/11b. 星号：单表 SELECT * / 单表 s1.*
--  12-14. 星号：多表 SELECT *、s1.* + 显式列、x.* + y.*
--  15. 星号 + 含 DROP 列的表（必须用真实 attnum）
--  16. 星号 + 自连接
--  17. USING 连接：按约定 fail-closed，锁住回退后的现状
--  17b. 普通查询里的 t.*（前端语法缺口修复的旁证）
--  18. 回环：解密的文件再 \copy FROM 回去
-- ============================================================

DROP TABLE IF EXISTS csj_a, csj_b, csj_c, csj_d CASCADE;
DROP DEK IF EXISTS dek_csj, dek_csj2 CASCADE;
DROP KEK IF EXISTS kek_csj CASCADE;

CREATE KEK kek_csj WITH (key_store = 'localkms', key_path = '/tmp/kek_csj.key');
CREATE DEK dek_csj WITH VALUES (KEK = kek_csj);
CREATE DEK dek_csj2 WITH VALUES (KEK = kek_csj);

CREATE TABLE csj_a (
    id   int,
    name text_enc ENCRYPTED WITH (DEK = dek_csj, SUPPORT_ALGO = 'det'),
    val  int4_enc ENCRYPTED WITH (DEK = dek_csj, SUPPORT_ALGO = 'ope')
);
CREATE TABLE csj_b (
    id    int,
    name  text_enc ENCRYPTED WITH (DEK = dek_csj, SUPPORT_ALGO = 'det'),
    score int4_enc ENCRYPTED WITH (DEK = dek_csj, SUPPORT_ALGO = 'ope')
);
-- csj_c / csj_d：第 3 列同为密态列但算法与 DEK 都不同
CREATE TABLE csj_c (
    id   int,
    name text_enc ENCRYPTED WITH (DEK = dek_csj, SUPPORT_ALGO = 'det'),
    val  int4_enc ENCRYPTED WITH (DEK = dek_csj, SUPPORT_ALGO = 'det')
);
CREATE TABLE csj_d (
    id    int,
    name  text_enc ENCRYPTED WITH (DEK = dek_csj2, SUPPORT_ALGO = 'ope'),
    score int4_enc ENCRYPTED WITH (DEK = dek_csj2, SUPPORT_ALGO = 'ope')
);

INSERT INTO csj_a VALUES (1, 'Alice', 10), (2, 'Bob', 20), (3, 'Carol', 30);
INSERT INTO csj_b VALUES (1, 'Alice', 100), (2, 'Bob', 200), (4, 'Dave', 400);
INSERT INTO csj_c VALUES (1, 'Cee1', 111), (2, 'Cee2', 222);
INSERT INTO csj_d VALUES (1, 'Dee1', 9001), (2, 'Dee2', 9002);

-- 基线：普通查询由客户端解密，两列都是明文
SELECT '0 plain', a.id, a.name, b.score FROM csj_a a JOIN csj_b b ON a.id = b.id ORDER BY a.id;

-- ============================================================
-- 1. 单表子查询导出（既有能力，对照）
-- ============================================================
\echo '=== 1 单表子查询导出（对照） ==='
\copy (SELECT id, name FROM csj_a ORDER BY id) TO 'csj_single.csv' CSV
\! cat csj_single.csv

-- ============================================================
-- 2. JOIN 子查询导出（原缺陷场景）：a.name 与 b.score 都应输出明文
-- ============================================================
\echo '=== 2 JOIN 子查询导出（原缺陷） ==='
\copy (SELECT a.id, a.name, b.score FROM csj_a a JOIN csj_b b ON a.id = b.id ORDER BY a.id) TO 'csj_join.csv' CSV
\! cat csj_join.csv

-- ============================================================
-- 3. 逗号连接：FROM 首项是真 RangeVar，修复前靠 attnum 巧合也能解
-- ============================================================
\echo '=== 3 逗号连接 ==='
\copy (SELECT a.id, a.name, b.score FROM csj_a a, csj_b b WHERE a.id = b.id ORDER BY a.id) TO 'csj_comma.csv' CSV
\! cat csj_comma.csv

-- ============================================================
-- 4. 镜像：FROM 首项换成另一张表
-- ============================================================
\echo '=== 4 镜像（FROM 首项换表） ==='
\copy (SELECT b.id, b.name, a.val FROM csj_b b JOIN csj_a a ON a.id = b.id ORDER BY b.id) TO 'csj_join_mirror.csv' CSV
\! cat csj_join_mirror.csv

-- ============================================================
-- 5. 同一张表的两种密态列
-- ============================================================
\echo '=== 5 同表两列不同算法 ==='
\copy (SELECT a.name, a.val FROM csj_a a ORDER BY a.id) TO 'csj_two_cols.csv' CSV
\! cat csj_two_cols.csv

-- ============================================================
-- 6. attnum 冲突 + 不同 DEK/算法：c.val 与 d.score 都是各自表的第 3 列
-- ============================================================
\echo '=== 6 attnum 冲突（不同 DEK/算法） ==='
\copy (SELECT c.id, c.val, d.score FROM csj_c c JOIN csj_d d ON c.id = d.id ORDER BY c.id) TO 'csj_collide.csv' CSV
\! cat csj_collide.csv

-- ============================================================
-- 7. LEFT JOIN（同一 JoinExpr 递归路径，右表可能为 NULL）
-- ============================================================
\echo '=== 7 LEFT JOIN ==='
\copy (SELECT a.id, a.name, d.score FROM csj_a a LEFT JOIN csj_d d ON a.id = d.id ORDER BY a.id) TO 'csj_left.csv' CSV
\! cat csj_left.csv

-- ============================================================
-- 8. 自连接：同一张表出现两次、别名不同
-- ============================================================
\echo '=== 8 自连接 ==='
\copy (SELECT x.id, x.name, y.name FROM csj_a x JOIN csj_a y ON x.id = y.id ORDER BY x.id) TO 'csj_self.csv' CSV
\! cat csj_self.csv

-- ============================================================
-- 9. JOIN + TEXT 格式（默认 tab 分隔）
-- ============================================================
\echo '=== 9 JOIN + TEXT 格式 ==='
\copy (SELECT a.id, a.name, b.score FROM csj_a a JOIN csj_b b ON a.id = b.id ORDER BY a.id) TO 'csj_join.txt'
\! cat csj_join.txt

-- ============================================================
-- 10. 三段限定列名（schema.表.列）：限定段必须是紧邻列名的那一段
-- ============================================================
\echo '=== 10 三段限定列名 ==='
\copy (SELECT public.csj_a.name, public.csj_b.score FROM public.csj_a JOIN public.csj_b ON csj_a.id = csj_b.id ORDER BY csj_a.id) TO 'csj_qual3.csv' CSV
\! cat csj_qual3.csv

-- ============================================================
-- 星号展开（SELECT * / t.*）—— 需要知道表的有序全列清单才能逐列定位，
-- 故单列出来一组用例；只覆盖 FROM 全是基表且无 USING/NATURAL 的情形。
-- ============================================================
CREATE TABLE csj_s1 (
    id   int,
    name text_enc ENCRYPTED WITH (DEK = dek_csj, SUPPORT_ALGO = 'det'),
    note text,
    val  int4_enc ENCRYPTED WITH (DEK = dek_csj, SUPPORT_ALGO = 'ope')
);
CREATE TABLE csj_s2 (
    id    int,
    tag   text_enc ENCRYPTED WITH (DEK = dek_csj, SUPPORT_ALGO = 'det'),
    score int4_enc ENCRYPTED WITH (DEK = dek_csj, SUPPORT_ALGO = 'ope'),
    extra text
);
-- csj_s3：中间列被 DROP（attnum 不连续：c=3 已丢弃，d=4、e=5）
CREATE TABLE csj_s3 (a int, b text_enc ENCRYPTED WITH (DEK = dek_csj, SUPPORT_ALGO = 'det'), c int, d int, e int4_enc ENCRYPTED WITH (DEK = dek_csj, SUPPORT_ALGO = 'ope'));
ALTER TABLE csj_s3 DROP COLUMN c;

INSERT INTO csj_s1 VALUES (1, 'S1a', 'n1', 41), (2, 'S1b', 'n2', 42);
INSERT INTO csj_s2 VALUES (1, 'T1', 51, 'e1'), (2, 'T2', 52, 'e2');
INSERT INTO csj_s3 (a, b, d, e) VALUES (1, 'S3a', 31, 61), (2, 'S3b', 32, 62);

-- 11. 单表星号（基线：修复前后都应对）
\echo '=== 11 单表 SELECT * ==='
\copy (SELECT * FROM csj_s1 ORDER BY id) TO 'csj_star_one.csv' CSV
\! cat csj_star_one.csv

-- 11b. 单表 + 限定星号（对照：与裸 * 只差一个 "s1." 前缀）
\echo '=== 11b 单表 s1.* ==='
\copy (SELECT s1.* FROM csj_s1 s1 ORDER BY s1.id) TO 'csj_star_qual_one.csv' CSV
\! cat csj_star_qual_one.csv

-- 12. 多表星号（原缺陷：JOIN 右表的密态列不解密）
\echo '=== 12 多表 SELECT * ==='
\copy (SELECT * FROM csj_s1 JOIN csj_s2 ON csj_s1.id = csj_s2.id ORDER BY csj_s1.id) TO 'csj_star_join.csv' CSV
\! cat csj_star_join.csv

-- 13. 限定星号 + 显式列
\echo '=== 13 s1.* + 显式列 ==='
\copy (SELECT s1.*, s2.score FROM csj_s1 s1 JOIN csj_s2 s2 ON s1.id = s2.id ORDER BY s1.id) TO 'csj_star_qual.csv' CSV
\! cat csj_star_qual.csv

-- 14. 两个限定星号
\echo '=== 14 x.* + y.* ==='
\copy (SELECT x.*, y.* FROM csj_s1 x JOIN csj_s2 y ON x.id = y.id ORDER BY x.id) TO 'csj_star_two.csv' CSV
\! cat csj_star_two.csv

-- 15. 含 DROP 列的表：星号展开必须用真实 attnum（跳过已丢弃列）
\echo '=== 15 含 DROP 列的 SELECT * ==='
\copy (SELECT * FROM csj_s3 ORDER BY a) TO 'csj_star_dropped.csv' CSV
\! cat csj_star_dropped.csv

-- 16. 自连接星号（同一张表出现两次）
\echo '=== 16 自连接 SELECT * ==='
\copy (SELECT * FROM csj_s1 x JOIN csj_s1 y ON x.id = y.id ORDER BY x.id) TO 'csj_star_self.csv' CSV
\! cat csj_star_self.csv

-- 17. USING 连接（PG 会把 join 列提到最前且只出现一次，无法按"各表顺序拼接"
--     展开）——按方案 fail-closed 维持现状，本用例锁住该回退行为
\echo '=== 17 USING 连接（fail-closed 回退） ==='
\copy (SELECT * FROM csj_s1 JOIN csj_s2 USING (id) ORDER BY id) TO 'csj_star_using.csv' CSV
\! cat csj_star_using.csv

-- 17b. 非 COPY 路径：t.* 的语法缺口修复旁证。
--      语法修复前 "s1.*" 整体解析失败 → TECC 对该语句完全不参与：t.* 取不到
--      解密、WHERE 里的密态常量也明文下发（对比会失败或报 invalid hex string）。
\echo '=== 17b 普通查询 t.*（语法修复旁证） ==='
SELECT s1.* FROM csj_s1 s1 ORDER BY s1.id;
SELECT '17b_where_const', count(*) FROM csj_s1 s1 WHERE s1.name = 'S1a';

-- ============================================================
-- 18. 回环：解密的文件再 \copy FROM 回去
-- ============================================================
\echo '=== 18 回环导入 ==='
TRUNCATE csj_a;
\copy csj_a (id, name) FROM 'csj_single.csv' WITH (FORMAT CSV)
SELECT '18 roundtrip', id, name FROM csj_a ORDER BY id;

-- ============================================================
-- 清理
-- ============================================================
DROP TABLE IF EXISTS csj_a, csj_b, csj_c, csj_d, csj_s1, csj_s2, csj_s3 CASCADE;
DROP DEK IF EXISTS dek_csj, dek_csj2 CASCADE;
DROP KEK IF EXISTS kek_csj CASCADE;
