--
-- bug_7939.sql — G10 -k模式下切换用户报错（已修复）
-- 根因: \c切换用户时TECC session state未正确清理
-- 修复: commit 7e48ba94940 (\c段错误修复)
--

DROP TABLE IF EXISTS test_switch CASCADE;
DROP DEK IF EXISTS dek_g7939 CASCADE;
DROP KEK IF EXISTS kek_g7939 CASCADE;

CREATE KEK kek_g7939 WITH (key_store = 'localkms', key_path = '/tmp/kek_g7939.key');
CREATE DEK dek_g7939 WITH VALUES(KEK = 'kek_g7939');

CREATE TABLE test_switch (
    id SERIAL PRIMARY KEY,
    val text_enc ENCRYPTED WITH (dek = dek_g7939, support_algo = 'det')
);

INSERT INTO test_switch (val) VALUES ('test_switch_user');

SELECT current_user AS before_switch;
SELECT val FROM test_switch;

-- 切换用户后应能正常查询
\c - test_postgres
SELECT current_user AS after_switch;

-- 切回原用户
\c - wxp
SELECT val FROM test_switch;

DROP TABLE test_switch CASCADE;
DROP DEK dek_g7939 CASCADE;
DROP KEK dek_g7939 CASCADE;
