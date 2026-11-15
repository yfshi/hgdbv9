--
-- execute_param_value.sql — libpq专有测试：EXECUTE 参数字面量按「值」加密
--
-- 参数路径与语句其余部分不同：它拿不到 AST 里 Const 的已求值内容，而是手工
-- 切分 EXECUTE 的参数文本后再加密。若把外层单引号一起当作明文（'alpha' 而非
-- alpha），会同时踩三个坑：
--   1) DET 等值查询恒不命中（静默返回 0 行，无任何报错）
--   2) 参数化 INSERT 落库的明文多出一对引号
--   3) 参数与字面量两条路径加密出的密文互不相认
-- 本用例按这三点逐条覆盖，并确认明文列参数与 OPE 列参数未被牵连。
--

DROP TABLE IF EXISTS test_epv CASCADE;
DROP DEK IF EXISTS dek_epv CASCADE;
DROP KEK IF EXISTS kek_epv CASCADE;

CREATE KEK kek_epv WITH (key_store = 'localkms', key_path = '/tmp/kek_epv.key');
CREATE DEK dek_epv WITH VALUES(KEK = 'kek_epv');

CREATE TABLE test_epv (
    id      INT4,
    val_det TEXT_ENC  ENCRYPTED WITH (DEK = dek_epv, SUPPORT_ALGO = 'DET'),
    val_ope INT4_ENC  ENCRYPTED WITH (DEK = dek_epv, SUPPORT_ALGO = 'OPE'),
    plain   VARCHAR(50)
);
INSERT INTO test_epv VALUES (1, 'alpha', 10, 'p1'), (2, 'beta', 20, 'p2');

SELECT '1_literal_baseline', id, val_det, val_ope, plain FROM test_epv ORDER BY id;

-- ============================================================
-- 2. 参数化等值查询：DET 必须命中，OPE 必须命中，不存在值必须为空
-- ============================================================
PREPARE epv_sel_det AS SELECT '2_param_det_hit' AS tag, id FROM test_epv WHERE val_det = $1;
EXECUTE epv_sel_det('alpha');
EXECUTE epv_sel_det('beta');
EXECUTE epv_sel_det('no_such_value');

PREPARE epv_sel_ope AS SELECT '3_param_ope_hit' AS tag, id FROM test_epv WHERE val_ope = $1;
EXECUTE epv_sel_ope(10);
EXECUTE epv_sel_ope(20);
EXECUTE epv_sel_ope(99);

-- ============================================================
-- 3. 参数化 INSERT：落库明文不得带引号
-- ============================================================
PREPARE epv_ins AS INSERT INTO test_epv VALUES ($1, $2, $3, $4);
EXECUTE epv_ins(3, 'gamma', 30, 'p3');
SELECT '4_param_insert_value', id, val_det, val_ope, plain FROM test_epv WHERE id = 3;

-- ============================================================
-- 4. 参数路径与字面量路径的密文必须互通
-- ============================================================
EXECUTE epv_sel_det('gamma');
SELECT '5_literal_sees_param_row', id FROM test_epv WHERE val_det = 'gamma';

INSERT INTO test_epv VALUES (4, 'delta', 40, 'p4');
EXECUTE epv_sel_det('delta');

-- ============================================================
-- 5. 参数文本的转义与 ::cast
-- ============================================================
EXECUTE epv_ins(5, E'esc\'aped', 50, 'p5');
SELECT '6_escape_string', id, val_det FROM test_epv WHERE id = 5;

EXECUTE epv_sel_det('alpha'::text);

EXECUTE epv_ins(6, 'quo''ted', 60, 'p6');
SELECT '7_embedded_quote', id, val_det FROM test_epv WHERE id = 6;
EXECUTE epv_sel_det('quo''ted');

-- ============================================================
-- 6. 明文列参数原样传递
-- ============================================================
EXECUTE epv_ins(7, 'epsilon', 70, 'p7');
SELECT '8_plain_param_intact', id, plain FROM test_epv WHERE id = 7;

-- ============================================================
-- 7. 参数带显式类型声明（$1::text_enc）：与隐式形态一样必须加密
-- ============================================================
PREPARE epv_cast AS SELECT '9_cast_param_hit' AS tag, id FROM test_epv WHERE val_det = $1::text_enc;
EXECUTE epv_cast('alpha');
EXECUTE epv_cast('no_such_value');

PREPARE epv_cast_rev AS SELECT '10_cast_param_left' AS tag, id FROM test_epv WHERE $1::text_enc = val_det;
EXECUTE epv_cast_rev('beta');

PREPARE epv_cast_ope AS SELECT '11_cast_ope_hit' AS tag, id FROM test_epv WHERE val_ope = $1::int4_enc;
EXECUTE epv_cast_ope(10);
EXECUTE epv_cast_ope(99);

-- ============================================================
-- 8. 派生表里的参数：限定名（别名）不是基表，须解析回基表才能加密
-- ============================================================
PREPARE epv_derived AS SELECT '12_derived_param_hit' AS tag, id FROM (SELECT id, val_det FROM test_epv) s WHERE s.val_det = $1;
EXECUTE epv_derived('alpha');
EXECUTE epv_derived('no_such_value');

PREPARE epv_derived_ope AS SELECT '13_derived_ope_param' AS tag, id FROM (SELECT id, val_ope FROM test_epv) s WHERE s.val_ope = $1;
EXECUTE epv_derived_ope(10);

DEALLOCATE ALL;
DROP TABLE IF EXISTS test_epv CASCADE;
DROP DEK IF EXISTS dek_epv CASCADE;
DROP KEK IF EXISTS kek_epv CASCADE;
