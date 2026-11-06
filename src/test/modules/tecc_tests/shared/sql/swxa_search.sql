-- ============================================================
-- swxa_search.sql — SWXA硬加密 SEARCH LIKE 模糊查询测试
-- ============================================================

-- 清理
DROP TABLE IF EXISTS swxa_search_t CASCADE;
DROP KEK IF EXISTS swxa_search_kek CASCADE;

SELECT '=== SWXA SEARCH Setup ===' AS stage;
CREATE KEK swxa_search_kek WITH (
    key_store = 'swxakms',
    key_path = 'hsm:id=1'
);
CREATE DEK swxa_search_dek WITH VALUES(KEK = swxa_search_kek);

-- ============================================================
-- 1. LIKE 通配符匹配
-- ============================================================
SELECT '=== 1 LIKE Wildcard ===' AS stage;

CREATE TABLE swxa_search_t (
    id      SERIAL,
    c_text  TEXT_ENC    ENCRYPTED WITH (DEK = swxa_search_dek, SUPPORT_ALGO = 'SEARCH'),
    c_vc    VARCHAR_ENC(100) ENCRYPTED WITH (DEK = swxa_search_dek, SUPPORT_ALGO = 'SEARCH'),
    c_bp    BPCHAR_ENC(50)   ENCRYPTED WITH (DEK = swxa_search_dek, SUPPORT_ALGO = 'SEARCH'),
    tag     TEXT
);

INSERT INTO swxa_search_t (c_text, c_vc, c_bp, tag) VALUES
    ('apple', 'apple', 'apple', 'r1'),
    ('application', 'application', 'application', 'r2'),
    ('banana', 'banana', 'banana', 'r3'),
    ('pineapple', 'pineapple', 'pineapple', 'r4'),
    ('Apple', 'Apple', 'Apple', 'r5'),
    ('APPLE', 'APPLE', 'APPLE', 'r6'),
    (NULL, NULL, NULL, 'r7');

-- 前缀匹配
SELECT '1a_text_like_app_pct' AS tc, id, c_text FROM swxa_search_t WHERE c_text LIKE 'app%' ORDER BY id;
SELECT '1b_vc_like_ban_pct' AS tc, id, c_vc FROM swxa_search_t WHERE c_vc LIKE 'ban%' ORDER BY id;

-- 后缀匹配
SELECT '1c_bp_like_ana_pct' AS tc, id, c_bp FROM swxa_search_t WHERE c_bp LIKE '%na' ORDER BY id;
SELECT '1d_text_like_ple_pct' AS tc, id, c_text FROM swxa_search_t WHERE c_text LIKE '%ple' ORDER BY id;

-- 包含匹配
SELECT '1e_text_like_app' AS tc, id, c_text FROM swxa_search_t WHERE c_text LIKE '%app%' ORDER BY id;
SELECT '1f_vc_like_ppl' AS tc, id, c_vc FROM swxa_search_t WHERE c_vc LIKE '%ppl%' ORDER BY id;

-- 清理
SELECT '=== SWXA SEARCH Cleanup ===' AS stage;
DROP TABLE swxa_search_t CASCADE;
DROP DEK swxa_search_dek CASCADE;
DROP KEK swxa_search_kek CASCADE;
