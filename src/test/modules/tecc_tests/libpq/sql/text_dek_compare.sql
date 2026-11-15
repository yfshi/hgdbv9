-- ============================================================
-- text_dek_compare.sql — 文本族密态类型的比较必须做跨 DEK 校验
--
-- 背景（已修复缺陷）：数值族（int2/int4/int8/float4/float8/numeric）的比较与
--   相等都汇聚到带 DEK 校验的公共函数上，而 text/varchar/bpchar 三类密态类型
--   各自实现了独立的相等与比较函数，这些实现只读密文段、没有校验；
--   NOT LIKE 也一样，只有 LIKE 走了校验。
--   于是「两个不同 DEK 的文本密态值」一旦真的走到服务端比较，就是静默错误：
--   等值比较静默 0 行、范围比较给出错误行数、排序把不能用同一把密钥解开的密文
--   混在一列返回（客户端批量解密时才报 item failed）。
--   前端对常见的「加密列 vs 加密列」写法已有拦截，但排序、以及把另一张表的值
--   写成标量子查询当右操作数这类形状，前端不视为两列比较，会直接落到服务端。
--
-- 修复：text_enc / varchar_enc 的相等与比较、bpchar_enc 的比较统一改走带
--   DEK 校验的公共实现（bpchar_enc 的相等此前已委托到带校验的实现），
--   NOT LIKE 补上与 LIKE 相同的校验。校验逻辑仍是同一处：比对两侧密文的
--   DEK 分布式身份标识，不一致报 ERRCODE_TECC_DEK_MISMATCH。
--
-- 判定点：
--   第 2 节三种文本类型的跨 DEK 排序、第 3 节跨 DEK 的等值/范围/NOT LIKE，
--   修复前都不报错（分别表现为客户端批量解密失败与静默错行数），修复后必须
--   报 "cannot compare encrypted values with different DEKs"。
--   第 1 节是同 DEK 对照：三类类型的排序、等值、范围、LIKE、NOT LIKE 都必须
--   照常工作，确保校验没有做成一刀切。
--   第 3 节最后一条跨 DEK 的 LIKE 是修复前就有的行为（LIKE 走的是已带校验的
--   实现），保留它用于确认 LIKE 与 NOT LIKE 现在的表现一致。
--
-- 归属：比较发生在服务端操作符函数内，与驱动无关；但这里刻意使用「前端不视为
--   两列比较」的语句形状来触达服务端，属 libpq 前端的行为，故放 libpq/sql
--   而不是 shared/sql。若日后前端也拦截这些形状，本用例的期望会从服务端
--   ERROR 变成前端 "TECC query preprocessing failed"，届时按新行为更新即可。
-- ============================================================

\set ON_ERROR_STOP off

-- ============================================================
-- 0. 幂等清理与建密钥：两个 DEK 交由同一把 KEK 派生，但身份标识不同
-- ============================================================
DROP TABLE IF EXISTS tdc_t1, tdc_t2, tdc_v1, tdc_v2, tdc_b1, tdc_b2,
                     tdc_s1, tdc_s2 CASCADE;
DROP DEK IF EXISTS tdc_d1 CASCADE;
DROP DEK IF EXISTS tdc_d2 CASCADE;
DROP KEK IF EXISTS tdc_k CASCADE;

\! rm -f "$TECC_KEK_PATH/tdc_k.key" 2>/dev/null
\! tecc_ktool generate --key-name tdc_k --key-algo sm4 --key-path "$TECC_KEK_PATH" >/dev/null 2>&1

CREATE KEK tdc_k WITH (
    key_store = 'localkms',
    key_algo  = 'sm4',
    key_path  = '%KEKPATH%/tdc_k.key'
);
CREATE DEK tdc_d1 WITH VALUES(KEK = 'tdc_k', KEY_ALGO = 'sm4');
CREATE DEK tdc_d2 WITH VALUES(KEK = 'tdc_k', KEY_ALGO = 'sm4');
-- 两把 DEK 的分布式身份标识必须不同，否则下面的比较本来就合法
SELECT '0 dek identity differs' AS t,
       count(DISTINCT column_key_distributed_id) AS n
  FROM hg_dek WHERE dek_name IN ('tdc_d1', 'tdc_d2');

-- ============================================================
-- 1. 建表：三类文本族各一对（d1 / d2），另加一对 SEARCH 列供 LIKE 使用
-- ============================================================
CREATE TABLE tdc_t1 (id int, v text_enc    ENCRYPTED WITH (DEK = 'tdc_d1', SUPPORT_ALGO = 'ope'));
CREATE TABLE tdc_t2 (id int, v text_enc    ENCRYPTED WITH (DEK = 'tdc_d2', SUPPORT_ALGO = 'ope'));
CREATE TABLE tdc_v1 (id int, v varchar_enc ENCRYPTED WITH (DEK = 'tdc_d1', SUPPORT_ALGO = 'ope'));
CREATE TABLE tdc_v2 (id int, v varchar_enc ENCRYPTED WITH (DEK = 'tdc_d2', SUPPORT_ALGO = 'ope'));
CREATE TABLE tdc_b1 (id int, v bpchar_enc  ENCRYPTED WITH (DEK = 'tdc_d1', SUPPORT_ALGO = 'ope'));
CREATE TABLE tdc_b2 (id int, v bpchar_enc  ENCRYPTED WITH (DEK = 'tdc_d2', SUPPORT_ALGO = 'ope'));
CREATE TABLE tdc_s1 (id int, v varchar_enc ENCRYPTED WITH (DEK = 'tdc_d1', SUPPORT_ALGO = 'search'));
CREATE TABLE tdc_s2 (id int, v varchar_enc ENCRYPTED WITH (DEK = 'tdc_d2', SUPPORT_ALGO = 'search'));
INSERT INTO tdc_t1 VALUES (1, 'aaa'), (2, 'bbb');
INSERT INTO tdc_t2 VALUES (3, 'aaa'), (4, 'ccc');
INSERT INTO tdc_v1 VALUES (1, 'aaa'), (2, 'bbb');
INSERT INTO tdc_v2 VALUES (3, 'aaa'), (4, 'ccc');
INSERT INTO tdc_b1 VALUES (1, 'aaa'), (2, 'bbb');
INSERT INTO tdc_b2 VALUES (3, 'aaa'), (4, 'ccc');
INSERT INTO tdc_s1 VALUES (1, 'aaa'), (2, 'bbb');
INSERT INTO tdc_s2 VALUES (3, 'aaa'), (4, 'ccc');

-- ============================================================
-- 2. 同 DEK 对照：三类类型的排序/等值/范围，以及 LIKE/NOT LIKE 必须照常
-- ============================================================
SELECT '=== 2 same DEK: must work ===';
SELECT '2 text sort' AS t, v FROM tdc_t1 ORDER BY v;
SELECT '2 varchar sort' AS t, v FROM tdc_v1 ORDER BY v;
SELECT '2 bpchar sort' AS t, v FROM tdc_b1 ORDER BY v;
SELECT '2 text eq' AS t, count(*) AS n FROM tdc_t1
 WHERE v = (SELECT v FROM tdc_t1 ORDER BY v LIMIT 1);
SELECT '2 varchar eq' AS t, count(*) AS n FROM tdc_v1
 WHERE v = (SELECT v FROM tdc_v1 ORDER BY v LIMIT 1);
SELECT '2 bpchar eq' AS t, count(*) AS n FROM tdc_b1
 WHERE v = (SELECT v FROM tdc_b1 ORDER BY v LIMIT 1);
SELECT '2 text lt' AS t, count(*) AS n FROM tdc_t1
 WHERE v < (SELECT v FROM tdc_t1 ORDER BY v DESC LIMIT 1);
SELECT '2 like const' AS t, count(*) AS n FROM tdc_s1 WHERE v LIKE 'a%';
SELECT '2 not like const' AS t, count(*) AS n FROM tdc_s1 WHERE v NOT LIKE 'a%';
SELECT '2 not like sub' AS t, count(*) AS n FROM tdc_s1
 WHERE v NOT LIKE (SELECT v FROM tdc_s1 ORDER BY id LIMIT 1);

-- ============================================================
-- 3. 跨 DEK 排序：三种类型都必须报错
--    （修复前：不报错，客户端批量解密时才发现 item failed）
-- ============================================================
SELECT '=== 3 cross DEK ordering: must be rejected ===';
SELECT '3 text sort' AS t, v
  FROM (SELECT v FROM tdc_t1 UNION ALL SELECT v FROM tdc_t2) s ORDER BY v;
SELECT '3 varchar sort' AS t, v
  FROM (SELECT v FROM tdc_v1 UNION ALL SELECT v FROM tdc_v2) s ORDER BY v;
SELECT '3 bpchar sort' AS t, v
  FROM (SELECT v FROM tdc_b1 UNION ALL SELECT v FROM tdc_b2) s ORDER BY v;

-- ============================================================
-- 4. 跨 DEK 等值/范围/LIKE/NOT LIKE：必须以标量子查询右操作数触达服务端并报错
--    （修复前：等值与范围静默给出错行数，NOT LIKE 静默给出错结果）
-- ============================================================
SELECT '=== 4 cross DEK equality/range/like: must be rejected ===';
SELECT '4 text eq' AS t, count(*) AS n FROM tdc_t1
 WHERE v = (SELECT v FROM tdc_t2 ORDER BY v LIMIT 1);
SELECT '4 text lt' AS t, count(*) AS n FROM tdc_t1
 WHERE v < (SELECT v FROM tdc_t2 ORDER BY v LIMIT 1);
SELECT '4 varchar eq' AS t, count(*) AS n FROM tdc_v1
 WHERE v = (SELECT v FROM tdc_v2 ORDER BY v LIMIT 1);
SELECT '4 bpchar eq' AS t, count(*) AS n FROM tdc_b1
 WHERE v = (SELECT v FROM tdc_b2 ORDER BY v LIMIT 1);
SELECT '4 like sub' AS t, count(*) AS n FROM tdc_s1
 WHERE v LIKE (SELECT v FROM tdc_s2 ORDER BY id LIMIT 1);
SELECT '4 not like sub' AS t, count(*) AS n FROM tdc_s1
 WHERE v NOT LIKE (SELECT v FROM tdc_s2 ORDER BY id LIMIT 1);

-- ============================================================
-- 5. 清理
-- ============================================================
SELECT '=== 5 cleanup ===';
DROP TABLE tdc_t1, tdc_t2, tdc_v1, tdc_v2, tdc_b1, tdc_b2, tdc_s1, tdc_s2 CASCADE;
DROP DEK tdc_d1 CASCADE;
DROP DEK tdc_d2 CASCADE;
DROP KEK tdc_k CASCADE;
SELECT '5 deks left' AS t, count(*) AS n FROM hg_dek WHERE dek_name LIKE 'tdc\_d%';
SELECT '5 keks left' AS t, count(*) AS n FROM hg_kek WHERE kek_name LIKE 'tdc\_k%';
