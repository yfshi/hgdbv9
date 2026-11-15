--
-- search_empty_string.sql — libpq专有测试：Bug 8171
--   SEARCH 算法列上 `LIKE ''`（空串）报错
--
-- 报障形态：
--   INSERT INTO t11(v) VALUES ('abc'), (''), ('xyz');
--   SELECT count(*) FROM t11 WHERE v LIKE '';   -- 预期 1，实际报错
--   → TECC ERROR: Invalid context for SEARCH algorithm
-- 根因：底层 tecc_search_sm4_encrypt() 本来就能处理 plain_len = 0（输出 4 字节
--   header、data_len = 0），但上层调度链有 4 处"长度判空即非法"的提前拦截
--   （tecc_algorithm_scheduler_encrypt_search_with_operator /
--   tecc_execute_algo_search_like_unified / tecc_execute_algo_search_equal /
--   tecc_algo_search_encrypt_prefix），外加 tecc_parse_pattern_full() 对
--   pattern_len == 0 返回 INVALID_PATTERN；空串还没进到底层就被拒。
-- 修复：上层只做指针非空校验，不预判长度；LIKE 遇空串时不走模式解析器，
--   直接按语义等价的精确匹配（空串的 LIKE 就是匹配空串本身）走精确加密路径。
--
-- 本用例锁：空串在 SEARCH 列上的 LIKE / NOT LIKE / 等值比较都要与 PG 明文语义
--   **逐条一致**（`'' LIKE '%'` 为真、`_` 不匹配空串、NULL 不参与匹配），
--   并覆盖写路径（INSERT/UPDATE 空串）与负例。
-- 断言方式：全部断"返回正确行数/正确行"，不只看"不报错"——空串若被当成
--   "无匹配"处理会静默返回 0 行，只断不报错是抓不到的。
--

DROP TABLE IF EXISTS t_ses CASCADE;
DROP DEK IF EXISTS dek_ses CASCADE;
DROP KEK IF EXISTS kek_ses CASCADE;

CREATE KEK kek_ses WITH (key_store = 'localkms', key_path = '%KEKPATH%/kek_ses.key');
CREATE DEK dek_ses WITH VALUES(KEK = 'kek_ses');

CREATE TABLE t_ses (
    id int,
    v  text_enc ENCRYPTED WITH (DEK = dek_ses, SUPPORT_ALGO = 'det,search')
);
INSERT INTO t_ses VALUES (1, 'abc'), (2, ''), (3, 'xyz'), (4, NULL);

SELECT '0_baseline_rows' AS tag, count(*) FROM t_ses;

-- ============================================================
-- 1. 报障原文：空串 LIKE（应与等值等价）
-- ============================================================
SELECT '1a_like_empty'   AS tag, count(*) FROM t_ses WHERE v LIKE '';
SELECT '1b_equal_empty'  AS tag, count(*) FROM t_ses WHERE v = '';
SELECT '1c_not_like_empty' AS tag, count(*) FROM t_ses WHERE v NOT LIKE '';

-- 命中的确实是"空串"这一行，不是 NULL 行：id = 2 且该行的值等于空串
SELECT '1d_hit_id'         AS tag, id FROM t_ses WHERE v LIKE '' ORDER BY id;
SELECT '1e_hit_is_empty'   AS tag, (v = '') AS is_empty FROM t_ses WHERE v LIKE '';
SELECT '1f_null_rows'      AS tag, count(*) FROM t_ses WHERE v IS NULL;

-- ============================================================
-- 2. 空串与通配符/PG 语义一致性（对照明文 PG 的行为）
-- ============================================================
SELECT '2a_like_percent'    AS tag, count(*) FROM t_ses WHERE v LIKE '%';
SELECT '2b_like_underscore' AS tag, count(*) FROM t_ses WHERE v LIKE '_';
SELECT '2c_like_exact'      AS tag, count(*) FROM t_ses WHERE v LIKE 'abc';
SELECT '2d_like_prefix'     AS tag, count(*) FROM t_ses WHERE v LIKE 'abc%';
SELECT '2e_like_suffix'     AS tag, count(*) FROM t_ses WHERE v LIKE '%c';
SELECT '2f_neg_no_match'    AS tag, count(*) FROM t_ses WHERE v LIKE 'no_such_value';

-- ============================================================
-- 3. 写路径：INSERT 空串 / UPDATE 空串
-- ============================================================
INSERT INTO t_ses VALUES (5, '');
SELECT '3a_after_insert_empty' AS tag, count(*) FROM t_ses WHERE v LIKE '';
SELECT '3b_ids_like_empty'     AS tag, id FROM t_ses WHERE v LIKE '' ORDER BY id;

UPDATE t_ses SET v = 'zzz' WHERE v LIKE '';
SELECT '3c_empty_after_update' AS tag, count(*) FROM t_ses WHERE v LIKE '';
SELECT '3d_zzz_after_update'   AS tag, count(*) FROM t_ses WHERE v = 'zzz';

-- ============================================================
-- 4. 清理
-- ============================================================
DROP TABLE IF EXISTS t_ses CASCADE;
DROP DEK IF EXISTS dek_ses CASCADE;
DROP KEK IF EXISTS kek_ses CASCADE;
SELECT 'z_cleanup_done' AS tag;
