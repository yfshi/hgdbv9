--
-- like_escape.sql — Review 问题15：LIKE / LIKE ESCAPE 功能验证
--

-- ============================================================
-- 0. Setup
-- ============================================================
DROP TABLE IF EXISTS t_esc CASCADE;
DROP DEK IF EXISTS dek_esc CASCADE;
DROP KEK IF EXISTS kek_esc CASCADE;

CREATE KEK kek_esc WITH (key_store = 'localkms', key_path = '/tmp/kek_esc.key');
CREATE DEK dek_esc WITH VALUES(KEK = kek_esc);

CREATE TABLE t_esc (
    id serial PRIMARY KEY,
    val varchar_enc ENCRYPTED WITH (DEK = 'dek_esc', support_algo = 'det,search')
);

INSERT INTO t_esc (val) VALUES ('abc_def');
INSERT INTO t_esc (val) VALUES ('abc%def');
INSERT INTO t_esc (val) VALUES ('abc\def');
INSERT INTO t_esc (val) VALUES ('abc#def');
INSERT INTO t_esc (val) VALUES ('abcXdef');
INSERT INTO t_esc (val) VALUES ('hello_world');

-- ============================================================
-- 1. 基础 LIKE（无 ESCAPE）—— 验证 SEARCH LIKE 基础功能
-- ============================================================
SELECT val FROM t_esc WHERE val LIKE 'abc_def';
SELECT val FROM t_esc WHERE val LIKE 'abc%def';
SELECT val FROM t_esc WHERE val LIKE 'hello%';
SELECT count(*) FROM t_esc WHERE val LIKE 'abc%';

-- ============================================================
-- 2. ESCAPE 基础验证（默认转义符 \）
-- ============================================================
-- \% 匹配字面量 %
SELECT val FROM t_esc WHERE val LIKE 'abc\%%' ESCAPE '\';

-- \_ 匹配字面量 _
SELECT val FROM t_esc WHERE val LIKE 'abc\_%' ESCAPE '\';

-- \\ 匹配字面量 \
SELECT val FROM t_esc WHERE val LIKE 'abc\\%' ESCAPE '\';

-- ============================================================
-- 3. 自定义转义符 #
-- ============================================================
SELECT val FROM t_esc WHERE val LIKE 'abc#%%' ESCAPE '#';
SELECT val FROM t_esc WHERE val LIKE 'abc#_%' ESCAPE '#';
SELECT val FROM t_esc WHERE val LIKE 'abc##%' ESCAPE '#';

-- ============================================================
-- 4. 清理
-- ============================================================
DROP TABLE IF EXISTS t_esc CASCADE;
DROP DEK IF EXISTS dek_esc;
DROP KEK IF EXISTS kek_esc;
