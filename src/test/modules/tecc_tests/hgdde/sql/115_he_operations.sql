-- ReqID:TECC4.0 - 同态加密运算在分布式形态下的语义（DDB-1.0-05）
-- Author: <wangxp@highgo.com>, 2026-10-11.
-- Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
--
-- 防护对象（DDB-1.0-05）：110 只固化了 he-add 的 SUM，本用例发散补齐同态运算的
--   全部形态在"每分片局部密文计算 → 协调节点合并 → 客户端解密"下的正确性：
--     - 加法同态（he-add）：列×列 加/减、与整型常量的标量加（常量在左/右）、标量乘
--       （列仅 he-add 时的 EC-ElGamal 标量倍点路径，常量以明文参与）；
--     - SUM 聚合（跨分片两阶段密文合并，结果提升 int8_enc）；
--     - 乘法同态（he-mul）：列×列、与整型常量的标量乘（常量由客户端持公钥加密）、
--       幂运算 ^ 与 power（指数为明文整数）；
--     - 跨类型运算：int4_enc + int8_enc（同 he-add DEK）→ int8_enc；
--     - 结果类型提升规则（pg_typeof）；
--     - 密态列 AVG 由客户端 fail-closed 拒绝。
--   同态 DEK 均为非对称 KEK（ddb_hekek, sm2）派生，顺带覆盖非对称密钥下的同态路径。
--   数据为 3 行、分片 8 个，逐行运算与聚合结果与分片落点无关。

\set ON_ERROR_STOP on
\set QUIET on
SET client_min_messages = warning;
-- hgdde 变体：分片数 GUC 为 hgdde.shard_count（与 Citus 的 citus.shard_count 同义）。
SET hgdde.shard_count = 8;

DROP TABLE IF EXISTS ddb_he CASCADE;

\echo '=== 5B.1 建表：he-add 两列、he-mul 两列、int8 he-add 一列（同 he-add DEK）并分发 ==='
CREATE TABLE ddb_he (
    id int,
    a  int4_enc ENCRYPTED WITH (HE_ADD_DEK = 'ddb_headd', SUPPORT_ALGO = 'he-add'),
    b  int4_enc ENCRYPTED WITH (HE_ADD_DEK = 'ddb_headd', SUPPORT_ALGO = 'he-add'),
    c  int4_enc ENCRYPTED WITH (HE_MUL_DEK = 'ddb_hemul', SUPPORT_ALGO = 'he-mul'),
    d  int4_enc ENCRYPTED WITH (HE_MUL_DEK = 'ddb_hemul', SUPPORT_ALGO = 'he-mul'),
    e  int8_enc ENCRYPTED WITH (HE_ADD_DEK = 'ddb_headd', SUPPORT_ALGO = 'he-add')
);
SELECT create_distributed_table('ddb_he', 'id');
INSERT INTO ddb_he (id, a, b, c, d, e) VALUES
    (1, 10, 3, 2, 5, 100),
    (2, 20, 4, 3, 7, 200),
    (3, 30, 5, 4, 11, 300);

\echo '=== 5B.2 加法同态：列×列 加/减 + 标量加（常量在右/左）+ 标量乘 ==='
SELECT id, a + b AS add_ab, a - b AS sub_ab, a + 100 AS add_right,
       100 + a AS add_left, a * 3 AS mul3
FROM ddb_he ORDER BY id;

\echo '=== 5B.3 加法同态 SUM（跨分片两阶段密文合并）==='
SELECT sum(a) AS sum_a, sum(b) AS sum_b FROM ddb_he;

\echo '=== 5B.4 乘法同态：列×列 + 标量乘（客户端加密常量）+ 幂（^ 与 power 等价）==='
SELECT id, c * d AS mul_cd, c * 5 AS mul5, c ^ 3 AS pow3, power(c, 3) AS pow3f
FROM ddb_he ORDER BY id;

\echo '=== 5B.5 跨类型运算：int4_enc + int8_enc（同 he-add DEK）→ int8_enc ==='
SELECT id, a + e AS cross_add FROM ddb_he ORDER BY id;

\echo '=== 5B.6 结果类型提升 ==='
SELECT pg_typeof(a + b)::text AS t_add,
       pg_typeof(a + e)::text AS t_cross,
       pg_typeof(c ^ 3)::text AS t_pow
FROM ddb_he ORDER BY id LIMIT 1;
SELECT pg_typeof(sum(a))::text AS t_sum FROM ddb_he;

\echo '=== 5B.7 密态列不支持 AVG（客户端 fail-closed）==='
\set ON_ERROR_STOP off
\set VERBOSITY terse
SELECT avg(a) FROM ddb_he;
\set VERBOSITY default
\set ON_ERROR_STOP on

DROP TABLE ddb_he;