--
-- set_op_paren.sql — libpq专有测试：集合运算的操作数带括号（嵌套集合表达式）
--
-- 前端语法（tecc_grammar.y）原来只在 UNION/INTERSECT/EXCEPT 的操作数位置接受
-- simple_select，缺少 PG14 的 select_with_parens。于是
--     (SELECT ... UNION ...) EXCEPT SELECT ... WHERE col = 'x'
--     SELECT ... EXCEPT (SELECT ... WHERE col = 'x')
-- 这类语句在前端整体解析失败（syntax error, unexpected '('），被
-- tecc_parser_mask_unparsable_stmts 掩码后原文下发，其中的密态常量不会被
-- 加密，服务端报 invalid hex string。
--
-- 本例覆盖：括号操作数（左/右/双侧/多层）、括号内的密态常量、括号内自带
-- ORDER BY/LIMIT，以及不带括号的对照组。
-- 注：括号操作数“之后”再跟 ORDER BY/LIMIT（如 (A) EXCEPT (B) ORDER BY id）
-- 仍不在前端语法内，属已知限制；需要确定性输出时用外层派生表包一层。
--

DROP TABLE IF EXISTS test_setop_a CASCADE;
DROP TABLE IF EXISTS test_setop_b CASCADE;
DROP DEK IF EXISTS dek_setop CASCADE;
DROP KEK IF EXISTS kek_setop CASCADE;

CREATE KEK kek_setop WITH (key_store = 'localkms', key_path = '/tmp/kek_setop.key');
CREATE DEK dek_setop WITH VALUES(KEK = 'kek_setop');

CREATE TABLE test_setop_a (
    id       INT4,
    val_det  TEXT_ENC ENCRYPTED WITH (DEK = dek_setop, SUPPORT_ALGO = 'DET'),
    val_ope  INT4_ENC ENCRYPTED WITH (DEK = dek_setop, SUPPORT_ALGO = 'OPE')
);
CREATE TABLE test_setop_b (
    id       INT4,
    val_det  TEXT_ENC ENCRYPTED WITH (DEK = dek_setop, SUPPORT_ALGO = 'DET'),
    val_ope  INT4_ENC ENCRYPTED WITH (DEK = dek_setop, SUPPORT_ALGO = 'OPE')
);
INSERT INTO test_setop_a VALUES (1, 'alpha', 10), (2, 'beta', 20), (3, 'gamma', 30);
INSERT INTO test_setop_b VALUES (1, 'alpha', 100), (2, 'beta', 200), (4, 'delta', 400);

SELECT '1_baseline' AS tag, id, val_det, val_ope FROM test_setop_a ORDER BY id;

-- ============================================================
-- 2. 对照组：不带括号的集合运算（本来就正常）
-- ============================================================
SELECT '2_plain_except' AS tag, id, val_det FROM test_setop_a
  EXCEPT SELECT '2_plain_except', id, val_det FROM test_setop_a WHERE val_det = 'alpha'
  ORDER BY id;

SELECT '3_plain_union' AS tag, id, val_det FROM test_setop_a
  UNION SELECT '3_plain_union', id, val_det FROM test_setop_b WHERE val_det = 'delta'
  ORDER BY id;

-- ============================================================
-- 4. 括号操作数：常量必须照常加密
-- ============================================================
-- 4.1 报障场景：左操作数是带括号的嵌套集合，右侧分支带常量
(SELECT id, val_det FROM test_setop_a UNION SELECT id, val_det FROM test_setop_b)
  EXCEPT SELECT id, val_det FROM test_setop_a WHERE val_det = 'alpha' ORDER BY id;

-- 4.2 只给左操作数加括号
(SELECT id, val_det FROM test_setop_a)
  EXCEPT SELECT id, val_det FROM test_setop_a WHERE val_det = 'alpha' ORDER BY id;

-- 4.3 只给右操作数加括号
SELECT id, val_det FROM (
  SELECT id, val_det FROM test_setop_a
  EXCEPT (SELECT id, val_det FROM test_setop_a WHERE val_det = 'alpha')
) s ORDER BY id;

-- 4.4 双侧加括号
SELECT id, val_det FROM (
  (SELECT id, val_det FROM test_setop_a)
  EXCEPT (SELECT id, val_det FROM test_setop_a WHERE val_det = 'alpha')
) s ORDER BY id;

-- 4.5 多层括号
SELECT id, val_det FROM (
  ((SELECT id, val_det FROM test_setop_a))
  EXCEPT (SELECT id, val_det FROM test_setop_a WHERE val_det = 'beta')
) s ORDER BY id;

-- 4.6 嵌套集合再包一层括号，两侧都带常量
SELECT id, val_det FROM (
  ((SELECT id, val_det FROM test_setop_a) UNION (SELECT id, val_det FROM test_setop_b WHERE val_det = 'delta'))
  EXCEPT (SELECT id, val_det FROM test_setop_a WHERE val_det = 'alpha')
) s ORDER BY id;

-- ============================================================
-- 5. 括号操作数 + 其它集合运算 / 算法
-- ============================================================
SELECT id, val_det FROM (
  (SELECT id, val_det FROM test_setop_a) INTERSECT ALL (SELECT id, val_det FROM test_setop_b)
) s ORDER BY id;

(SELECT id, val_det FROM test_setop_a)
  UNION ALL SELECT id, val_det FROM test_setop_b WHERE val_det = 'delta' ORDER BY id;

SELECT id, val_ope FROM (
  (SELECT id, val_ope FROM test_setop_a)
  EXCEPT (SELECT id, val_ope FROM test_setop_a WHERE val_ope = 20)
) s ORDER BY id;

-- ============================================================
-- 6. 括号内自带 ORDER BY / LIMIT（子句在括号内，仍走 simple_select）
-- ============================================================
SELECT id, val_det FROM (
  (SELECT id, val_det FROM test_setop_a ORDER BY id LIMIT 2)
  EXCEPT (SELECT id, val_det FROM test_setop_a WHERE val_det = 'beta')
) s ORDER BY id;

-- ============================================================
-- 7. 括号操作数 + 无常量（对照组：结果集本身要正确）
-- ============================================================
SELECT id, val_det FROM (
  (SELECT id, val_det FROM test_setop_a) UNION (SELECT id, val_det FROM test_setop_b)
) s ORDER BY id;

-- 括号里的子查询单独使用（标量子查询上下文）
SELECT '8_paren_sublink' AS tag, val_det FROM test_setop_a
  WHERE val_det = (SELECT val_det FROM test_setop_a WHERE id = 2);

DROP TABLE IF EXISTS test_setop_a CASCADE;
DROP TABLE IF EXISTS test_setop_b CASCADE;
DROP DEK IF EXISTS dek_setop CASCADE;
DROP KEK IF EXISTS kek_setop CASCADE;
