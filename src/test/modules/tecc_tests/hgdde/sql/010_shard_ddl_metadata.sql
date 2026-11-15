-- ReqID:TECC4.0 - 分片 DDL 带出 ENCRYPTED WITH（W2 分布式护栏）
-- Author: <wangxp@highgo.com>, 2026-10-05.
-- Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
--
-- 防护对象（W2，plan/2026-10-28-hgdde分布式适配可行性分析.md §0.3）：
--   分发加密表时，插件下发的 worker 侧 DDL 是**按元组描述符反解析**出来的，
--   ENCRYPTED WITH 只存在 hg_encrypted_columns、不在 pg_attribute 里，所以文本里
--   没有这个子句。现在的补法在 **libpq 侧**（`TECC_MODE_DDL_ONLY`）：TECC 的
--   DDL 处理在下发前把子句合成进文本（形态是
--   `SELECT worker_apply_shard_ddl_command(<shardid>, '<DDL>')`，改写的是内层文本）；
--   worker 侧执行后再由 DefineRelation 按**分片自己的 OID** 落一行
--   hg_encrypted_columns。**这一步已经不再依赖 Citus 的反解析器**——`contrib/citus`
--   里那两处 deparser 改动（citus_ruleutils.c / deparse_table_stmts.c）已按新策略回退，
--   本用例正是在"vanilla Citus 语义"下验证的。
--   这一步退化的话，分片就是「列有加密类型、查不到元数据」的静默降级状态
--   （W2b 之前不报错，查询结果可能错）。
--
-- 断言口径：分片元数据指纹 = `元数据行数|逐列 列名:加密类型位掩码:DEK分布式身份标识`。
--   用 distributed_id 而不是 DEK 的 OID：OID 是各节点本地对象号，跨节点必然不同；
--   distributed_id 按 `schema.dekname` 哈希生成（createdek.c），天生跨节点一致。
--   加密类型位掩码：det=1, ope=2, search=4, he-add=8, he-mul=16。
--
-- 四个密态列各对应一条真实缺口：
--   a  int4_enc  DEK + HE_ADD_DEK  det,he-add   混合算法 + 同态绑定
--   c  int4_enc  只有 HE_ADD_DEK   he-add       纯同态：DEK 选项必须不出现（dekn=0）
--   d  varchar_enc(20)  search                  字符列带 typmod；**非 OPE 字符列**。
--                                               vanilla Citus 会照样下发 COLLATE，
--                                               所以这一列同时是"插入点必须在类型之后、
--                                               COLLATE 之前"的判别点（插错位置直接语法错）。
--   t  text_enc  ope                            OPE 字符列：同上
--   id int                                      分布键（明文，无元数据行）

\set ON_ERROR_STOP on
\set QUIET on
SET client_min_messages = warning;
-- 分片数固定 8：默认 32 会让下面每条按分片下发的断言开 32 条内部连接，慢且易撞
-- worker 的 max_connections；分片数不影响本用例要防护的「子句渲染」路径。
SET hgdde.shard_count = 8;

DROP TABLE IF EXISTS ddb_meta CASCADE;

\echo '=== 1.1 建表并分发（明文分布键）==='
CREATE TABLE ddb_meta (
    id int,
    a  int4_enc  ENCRYPTED WITH (DEK = 'ddb_dek', HE_ADD_DEK = 'ddb_headd', SUPPORT_ALGO = 'det,he-add'),
    c  int4_enc  ENCRYPTED WITH (HE_ADD_DEK = 'ddb_headd', SUPPORT_ALGO = 'he-add'),
    d  varchar_enc(20) ENCRYPTED WITH (DEK = 'ddb_dek', SUPPORT_ALGO = 'search'),
    t  text_enc  ENCRYPTED WITH (DEK = 'ddb_dek', SUPPORT_ALGO = 'ope')
);
SELECT create_distributed_table('ddb_meta', 'id');

\echo '=== 1.2 分片数量与落点 ==='
-- hgdde's pg_dist_shard keeps only (logicalrelid, shardid, shardstorage, shardmin/maxvalue)；
-- nodename/nodeport 在 pg_dist_shard_placement 里。节点数按 nodeport 数。
SELECT count(*) AS shards, count(DISTINCT p.nodeport) AS nodes
FROM pg_dist_shard s JOIN pg_dist_shard_placement p USING (shardid)
WHERE s.logicalrelid = 'ddb_meta'::regclass;

\echo '=== 1.3 每个分片自己的密态元数据指纹（必须完全一致）==='
SELECT count(*) AS placements,
       count(*) FILTER (WHERE NOT success) AS failed,
       count(DISTINCT result) AS distinct_fingerprint,
       min(result) AS shard_fingerprint
FROM run_command_on_placements('ddb_meta'::regclass, $fp$
    WITH s AS (SELECT '%s'::regclass::oid AS oid)
    SELECT (SELECT count(*) FROM hg_encrypted_columns WHERE table_oid = s.oid)::text || '|' ||
           coalesce((SELECT string_agg(a.attname || ':' || ec.encryption_type || ':' ||
                              coalesce(k.column_key_distributed_id::text, '0'), ',' ORDER BY a.attnum)
                     FROM pg_attribute a
                     JOIN hg_encrypted_columns ec
                       ON ec.table_oid = a.attrelid AND ec.column_attnum = a.attnum
                     LEFT JOIN hg_dek k ON k.oid = ec.column_key_id
                     WHERE a.attrelid = s.oid AND a.attnum > 0 AND NOT a.attisdropped), '(none)')
    FROM s
$fp$);

\echo '=== 1.4 三处来源指纹一致：分片 / worker 上的父表副本 / coordinator 上的父表 ==='
-- worker 上的父表副本由 Citus 的 metadata sync 建出，走的是**同一个**子句生成器
-- （metadata_sync.c）；三方放在一起比对，分母应是 8（分片）+ 2（worker 父表副本）
-- + 1（coordinator 父表）= 11，分子必须是 1。
SELECT count(DISTINCT fp) AS distinct_fingerprints, count(*) AS sources, min(fp) AS fingerprint
FROM (
    SELECT result AS fp
    FROM run_command_on_placements('ddb_meta'::regclass, $fp$
        WITH s AS (SELECT '%s'::regclass::oid AS oid)
        SELECT (SELECT count(*) FROM hg_encrypted_columns WHERE table_oid = s.oid)::text || '|' ||
               coalesce((SELECT string_agg(a.attname || ':' || ec.encryption_type || ':' ||
                                  coalesce(k.column_key_distributed_id::text, '0'), ',' ORDER BY a.attnum)
                         FROM pg_attribute a
                         JOIN hg_encrypted_columns ec
                           ON ec.table_oid = a.attrelid AND ec.column_attnum = a.attnum
                         LEFT JOIN hg_dek k ON k.oid = ec.column_key_id
                         WHERE a.attrelid = s.oid AND a.attnum > 0 AND NOT a.attisdropped), '(none)')
        FROM s
    $fp$)
    UNION ALL
    SELECT result
    FROM run_command_on_workers($wp$
        WITH s AS (SELECT 'public.ddb_meta'::regclass::oid AS oid)
        SELECT (SELECT count(*) FROM hg_encrypted_columns WHERE table_oid = s.oid)::text || '|' ||
               coalesce((SELECT string_agg(a.attname || ':' || ec.encryption_type || ':' ||
                                  coalesce(k.column_key_distributed_id::text, '0'), ',' ORDER BY a.attnum)
                         FROM pg_attribute a
                         JOIN hg_encrypted_columns ec
                           ON ec.table_oid = a.attrelid AND ec.column_attnum = a.attnum
                         LEFT JOIN hg_dek k ON k.oid = ec.column_key_id
                         WHERE a.attrelid = s.oid AND a.attnum > 0 AND NOT a.attisdropped), '(none)')
        FROM s
    $wp$)
    UNION ALL
    SELECT (SELECT count(*) FROM hg_encrypted_columns WHERE table_oid = 'ddb_meta'::regclass)::text || '|' ||
           coalesce((SELECT string_agg(a.attname || ':' || ec.encryption_type || ':' ||
                              coalesce(k.column_key_distributed_id::text, '0'), ',' ORDER BY a.attnum)
                     FROM pg_attribute a
                     JOIN hg_encrypted_columns ec
                       ON ec.table_oid = a.attrelid AND ec.column_attnum = a.attnum
                     LEFT JOIN hg_dek k ON k.oid = ec.column_key_id
                     WHERE a.attrelid = 'ddb_meta'::regclass
                       AND a.attnum > 0 AND NOT a.attisdropped), '(none)')
) t;

\echo '=== 1.5 字符类加密列的排序规则（fork 语义 + COLLATE 抑制的落地形态）==='
-- 加密字符列由 fork 统一给 "C" 排序规则（与 locale 无关），字面量 COLLATE 只被
-- 允许出现在含 ope 的列上。这里固定住分片上的实际取值，并断言分片列与同节点
-- 父表副本列的排序规则一致。
SELECT count(DISTINCT result) AS distinct_fingerprint, min(result) AS collation_check
FROM run_command_on_placements('ddb_meta'::regclass, $co$
    SELECT string_agg(x.attname || ':' || (x.attcollation = y.attcollation)::text || ':' ||
                      x.attcollation::regcollation::text, ',' ORDER BY x.attname)
    FROM pg_attribute x
    JOIN pg_attribute y ON y.attrelid = 'public.ddb_meta'::regclass AND y.attname = x.attname
    WHERE x.attrelid = '%s'::regclass AND x.attname IN ('c', 'd', 't') AND x.attnum > 0
$co$);
