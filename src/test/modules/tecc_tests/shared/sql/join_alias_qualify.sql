-- ============================================================
-- join_alias_qualify.sql — JOIN 下带别名限定的常量加密取表回归
--
-- 背景：表达式改写解析列引用归属时，原顺序是「直查限定名 → defaultTable 兜底
-- → FROM 别名映射」。JOIN 下 defaultTable 只是 FROM 左表，当左右两表存在
-- 同名列但密态类型不同时，右表别名限定列（如 i.val_ope）会先被 defaultTable
-- 兜底误判成左表的同名列，常量按错误的密态类型加密，服务端报
-- operator does not exist: int4_enc >= text_enc（或反之）。
-- 修复：限定名是已登记的 FROM 别名时封死 defaultTable 通道，改由别名映射解析。
--
-- 本用例覆盖：
--   1. 单表基线（修复前后都应通过）
--   2. JOIN + 左表别名限定（修复前后都应通过，靠 defaultTable 侥幸命中）
--   3. JOIN + 右表别名限定的范围/等值比较（原缺陷场景）
--   4. 对调 JOIN 顺序的镜像场景（defaultTable 换成另一张表后同样暴露）
-- ============================================================

-- ============================================================
-- 0. Setup — 共享 KEK/DEK，两表同名列但密态类型不同
-- ============================================================
DROP KEK IF EXISTS aj_kek CASCADE;
CREATE KEK aj_kek WITH (key_store='localkms', key_path='/tmp/aj_kek.key');
CREATE DEK aj_dek WITH VALUES(KEK=aj_kek);

DROP TABLE IF EXISTS aj_t_txt, aj_t_int CASCADE;
-- aj_t_txt: val_det/val_ope 为 text_enc（与整型表同名列、密态类型不同）
CREATE TABLE aj_t_txt (
    id int,
    val_det text_enc ENCRYPTED WITH (DEK='aj_dek', SUPPORT_ALGO='det'),
    val_ope text_enc ENCRYPTED WITH (DEK='aj_dek', SUPPORT_ALGO='ope'),
    val_search text_enc ENCRYPTED WITH (DEK='aj_dek', SUPPORT_ALGO='search')
);
-- aj_t_int: val_det/val_ope 为 int4_enc
CREATE TABLE aj_t_int (
    id int,
    val_det int4_enc ENCRYPTED WITH (DEK='aj_dek', SUPPORT_ALGO='det'),
    val_ope int4_enc ENCRYPTED WITH (DEK='aj_dek', SUPPORT_ALGO='ope')
);

INSERT INTO aj_t_txt (id, val_det, val_ope, val_search) VALUES
(1, 'a50', 'a50', 'Apple group'),
(2, 'b60', 'b60', 'Banana group'),
(3, 'c70', 'c70', 'Cherry group');
INSERT INTO aj_t_int (id, val_det, val_ope) VALUES
(1, 50, 50),
(2, 60, 60),
(3, 70, 70);

-- ============================================================
-- 1. 单表基线
-- ============================================================
SELECT '=== 1 单表基线 ===';
SELECT '1a_int_range', count(*) FROM aj_t_int i WHERE i.val_ope >= 60;
SELECT '1b_txt_like', count(*) FROM aj_t_txt t WHERE t.val_search LIKE 'A%';
SELECT '1c_int_eq', count(*) FROM aj_t_int i WHERE i.val_det = 60;
SELECT '1d_txt_range', count(*) FROM aj_t_txt t WHERE t.val_ope >= 'b50';

-- ============================================================
-- 2. JOIN + 左表别名限定（defaultTable 与限定目标同表，侥幸正确）
-- ============================================================
SELECT '=== 2 JOIN 左表别名限定 ===';
SELECT '2a_txt_range', count(*) FROM aj_t_txt t JOIN aj_t_int i ON i.id = t.id WHERE t.val_ope >= 'b50';
SELECT '2b_txt_like', count(*) FROM aj_t_txt t JOIN aj_t_int i ON i.id = t.id WHERE t.val_search LIKE 'B%';

-- ============================================================
-- 3. JOIN + 右表别名限定（原缺陷场景，defaultTable=左表 text 同名列误命中）
-- ============================================================
SELECT '=== 3 JOIN 右表别名限定（原缺陷场景） ===';
SELECT '3a_int_range', count(*) FROM aj_t_txt t JOIN aj_t_int i ON i.id = t.id WHERE i.val_ope >= 60;
SELECT '3b_int_eq', count(*) FROM aj_t_txt t JOIN aj_t_int i ON i.id = t.id WHERE i.val_det = 60;
SELECT '3c_mixed', count(*) FROM aj_t_txt t JOIN aj_t_int i ON i.id = t.id
    WHERE t.val_search LIKE 'B%' AND i.val_ope >= 60 AND i.val_ope < 70;

-- ============================================================
-- 4. 对调 JOIN 顺序的镜像场景（defaultTable 换成整型表后，左表别名 t 的
--    text 列同样会被整型表同名列误命中）
-- ============================================================
SELECT '=== 4 对调 JOIN 顺序（镜像） ===';
SELECT '4a_int_range', count(*) FROM aj_t_int i JOIN aj_t_txt t ON i.id = t.id WHERE i.val_ope >= 60;
SELECT '4b_txt_range', count(*) FROM aj_t_int i JOIN aj_t_txt t ON i.id = t.id WHERE t.val_ope >= 'b50';
SELECT '4c_txt_eq', count(*) FROM aj_t_int i JOIN aj_t_txt t ON i.id = t.id WHERE t.val_det = 'b60';

-- ============================================================
-- 5. 清理
-- ============================================================
DROP TABLE IF EXISTS aj_t_txt, aj_t_int CASCADE;
DROP DEK IF EXISTS aj_dek CASCADE;
DROP KEK IF EXISTS aj_kek CASCADE;
SELECT '=== 清理完成 ===';
