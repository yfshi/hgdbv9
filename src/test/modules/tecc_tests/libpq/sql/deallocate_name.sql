--
-- deallocate_name.sql — libpq专有测试：DEALLOCATE <name> 与同名 PREPARE 重定义
--
-- 前端扫描器一度未产出 DEALLOCATE 关键字 token，`DEALLOCATE name` 被解析成
-- "标识符 标识符"而报语法错，兜底路径又把它当成 SELECT 放过，于是
-- processor_handle_deallocate_statement 从未执行、客户端 prepared statement
-- 记录不释放；同名再次 PREPARE 时会命中 "already exists" 并沿用旧的列映射，
-- 使参数被当作明文外发（加密列报 invalid hex string / cannot be coerced）。
--
-- 本用例覆盖四种 DEALLOCATE 形态，并验证：
--   1) 同名重定义（先给明文表用、DEALLOCATE 后改给加密表用）后参数按新列加密；
--   2) 加密列的 DET / OPE 参数化等值查询命中；
--   3) 名字未释放时重复 PREPARE，仍由服务端报 already exists（前端不静默改写）。
--

DROP TABLE IF EXISTS test_dl_plain CASCADE;
DROP TABLE IF EXISTS test_dl_enc CASCADE;
DROP DEK IF EXISTS dek_dl CASCADE;
DROP KEK IF EXISTS kek_dl CASCADE;

CREATE KEK kek_dl WITH (key_store = 'localkms', key_path = '/tmp/kek_dl.key');
CREATE DEK dek_dl WITH VALUES(KEK = 'kek_dl');

-- 明文表：用来先占用名字
CREATE TABLE test_dl_plain (id INT4, name VARCHAR(50), salary INT4, dept VARCHAR(50));
-- 加密表：同名重定义后必须按这里的列加密
CREATE TABLE test_dl_enc (
    id      INT4,
    name    VARCHAR_ENC(100) ENCRYPTED WITH (DEK = dek_dl, SUPPORT_ALGO = 'DET'),
    salary  INT4_ENC         ENCRYPTED WITH (DEK = dek_dl, SUPPORT_ALGO = 'OPE'),
    dept    VARCHAR(50)
);

SELECT '1_setup', count(*) FROM test_dl_enc;

-- ============================================================
-- 2. 四种 DEALLOCATE 形态都不得报语法错
-- ============================================================
PREPARE d_a (int4, varchar, int4, varchar) AS INSERT INTO test_dl_plain VALUES ($1, $2, $3, $4);
DEALLOCATE d_a;

PREPARE d_b (int4, varchar, int4, varchar) AS INSERT INTO test_dl_plain VALUES ($1, $2, $3, $4);
DEALLOCATE PREPARE d_b;

PREPARE d_c (int4, varchar, int4, varchar) AS INSERT INTO test_dl_plain VALUES ($1, $2, $3, $4);
DEALLOCATE ALL;

PREPARE d_d (int4, varchar, int4, varchar) AS INSERT INTO test_dl_plain VALUES ($1, $2, $3, $4);
DEALLOCATE PREPARE ALL;

-- ============================================================
-- 3. 同名重定义：先明文表、释放、再给加密表
-- ============================================================
PREPARE d_reuse (int4, varchar, int4, varchar) AS INSERT INTO test_dl_plain VALUES ($1, $2, $3, $4);
EXECUTE d_reuse(1, 'plain row', 100, 'P');
SELECT '2_plain_row', id, name FROM test_dl_plain ORDER BY id;
DEALLOCATE d_reuse;

PREPARE d_reuse (int4, varchar_enc, int4_enc, varchar) AS INSERT INTO test_dl_enc VALUES ($1, $2, $3, $4);
EXECUTE d_reuse(1, 'Alice', 50000, 'IT');
SELECT '3_enc_row_after_reuse', id, name, salary FROM test_dl_enc ORDER BY id;

PREPARE d_sel (varchar_enc) AS SELECT '4_det_param_hit' AS tag, id FROM test_dl_enc WHERE name = $1;
EXECUTE d_sel('Alice');
DEALLOCATE d_sel;

PREPARE d_sel2 (int4_enc) AS SELECT '5_ope_param_hit' AS tag, id FROM test_dl_enc WHERE salary = $1;
EXECUTE d_sel2(50000);
DEALLOCATE d_sel2;

DEALLOCATE d_reuse;

-- ============================================================
-- 4. 名字未释放时重复 PREPARE：服务端仍应报 already exists
-- ============================================================
PREPARE d_keep AS SELECT id FROM test_dl_enc;
PREPARE d_keep AS SELECT id FROM test_dl_enc;
DEALLOCATE d_keep;

DEALLOCATE ALL;
DROP TABLE IF EXISTS test_dl_plain CASCADE;
DROP TABLE IF EXISTS test_dl_enc CASCADE;
DROP DEK IF EXISTS dek_dl CASCADE;
DROP KEK IF EXISTS kek_dl CASCADE;
