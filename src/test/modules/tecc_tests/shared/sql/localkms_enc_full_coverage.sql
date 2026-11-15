-- ============================================================
-- TECC 软加密（localkms）全覆盖用例
-- 覆盖9种加密类型、4种算法（RND/DET/OPE/SEARCH）
-- 包含多行INSERT（多种写法）、全场景WHERE查询、UPDATE、DELETE
--
-- 放 shared/：本用例只用普通 SQL（无 psql 元命令、无美元引用、注释与字符串内无分号），
-- 两个驱动都能执行 —— libpq 侧走 psql（001），JDBC 侧走 TeccSqlTestRunner（002）。
-- 原在 jdbc/sql/test_jdbc_soft_enc_full_coverage.sql：该目录的用例其实由 psql 执行
-- （run_sql_dir_diff → run_case_sql_via_psql），Java 驱动从未参与，目录名与归类失真。
--
-- 「DEFAULT 值表」子场景（Part 3.8 / Part 4.17）曾因客户端负缓存哨兵缺陷在 libpq 侧
-- 报 4 条错误（详见 plan/2026-09-20-加密列带DEFAULT的软加密表libpq入参加密失效.md），
-- 该缺陷已修复，两侧驱动输出均已正常。
-- ============================================================

-- ============================================================
-- Part 1: 环境准备
-- ============================================================
DROP TABLE IF EXISTS localkms_all_enc CASCADE;
DROP TABLE IF EXISTS localkms_det_tbl CASCADE;
DROP TABLE IF EXISTS localkms_ope_tbl CASCADE;
DROP TABLE IF EXISTS localkms_search_tbl CASCADE;
DROP TABLE IF EXISTS localkms_default_tbl CASCADE;
DROP DEK IF EXISTS localkms_dek;
DROP KEK IF EXISTS localkms_kek;

CREATE KEK localkms_kek WITH (
    key_store = 'localkms',
    key_path = '/tmp/localkms_kek.key'
);

CREATE DEK localkms_dek WITH VALUES(
    KEK = localkms_kek
);

-- ============================================================
-- Part 2: 建表（覆盖所有加密类型和算法）
-- ============================================================

-- 表1: 全覆盖表 — 9种加密类型，混合算法（RND/DET/OPE/SEARCH）
SELECT '=== Part 2.1: 创建全覆盖表 ===' AS stage;

CREATE TABLE localkms_all_enc (
    id        SERIAL PRIMARY KEY,
    -- 数值类型 — DET（仅解密，不做WHERE查询）
    c_int2    INT2_ENC    ENCRYPTED WITH (DEK = localkms_dek, SUPPORT_ALGO = 'DET'),
    c_int4    INT4_ENC    ENCRYPTED WITH (DEK = localkms_dek, SUPPORT_ALGO = 'DET'),
    c_int8    INT8_ENC    ENCRYPTED WITH (DEK = localkms_dek, SUPPORT_ALGO = 'DET'),
    -- 数值类型 — DET + OPE（等值 + 范围）
    c_f4_det  FLOAT4_ENC  ENCRYPTED WITH (DEK = localkms_dek, SUPPORT_ALGO = 'DET'),
    c_f8_ope  FLOAT8_ENC  ENCRYPTED WITH (DEK = localkms_dek, SUPPORT_ALGO = 'OPE'),
    c_num_ope NUMERIC_ENC ENCRYPTED WITH (DEK = localkms_dek, SUPPORT_ALGO = 'OPE'),
    -- 字符类型 — DET + SEARCH
    c_vc_det  VARCHAR_ENC(100) ENCRYPTED WITH (DEK = localkms_dek, SUPPORT_ALGO = 'DET'),
    c_bp_srch BPCHAR_ENC(50)   ENCRYPTED WITH (DEK = localkms_dek, SUPPORT_ALGO = 'DET,SEARCH'),
    c_txt_all TEXT_ENC         ENCRYPTED WITH (DEK = localkms_dek, SUPPORT_ALGO = 'OPE,SEARCH'),
    -- 标记列
    tag       TEXT
);

-- 表2: DET专项表 — 等值查询
SELECT '=== Part 2.2: 创建DET专项表 ===' AS stage;

CREATE TABLE localkms_det_tbl (
    id       SERIAL PRIMARY KEY,
    det_i2   INT2_ENC    ENCRYPTED WITH (DEK = localkms_dek, SUPPORT_ALGO = 'DET'),
    det_i4   INT4_ENC    ENCRYPTED WITH (DEK = localkms_dek, SUPPORT_ALGO = 'DET'),
    det_i8   INT8_ENC    ENCRYPTED WITH (DEK = localkms_dek, SUPPORT_ALGO = 'DET'),
    det_f4   FLOAT4_ENC  ENCRYPTED WITH (DEK = localkms_dek, SUPPORT_ALGO = 'DET'),
    det_f8   FLOAT8_ENC  ENCRYPTED WITH (DEK = localkms_dek, SUPPORT_ALGO = 'DET'),
    det_num  NUMERIC_ENC ENCRYPTED WITH (DEK = localkms_dek, SUPPORT_ALGO = 'DET'),
    det_vc   VARCHAR_ENC(100) ENCRYPTED WITH (DEK = localkms_dek, SUPPORT_ALGO = 'DET'),
    det_bp   BPCHAR_ENC(50)   ENCRYPTED WITH (DEK = localkms_dek, SUPPORT_ALGO = 'DET'),
    det_txt  TEXT_ENC         ENCRYPTED WITH (DEK = localkms_dek, SUPPORT_ALGO = 'DET'),
    tag      TEXT
);

-- 表3: OPE专项表 — 范围查询 + ORDER BY
SELECT '=== Part 2.3: 创建OPE专项表 ===' AS stage;

CREATE TABLE localkms_ope_tbl (
    id       SERIAL PRIMARY KEY,
    ope_i2   INT2_ENC    ENCRYPTED WITH (DEK = localkms_dek, SUPPORT_ALGO = 'OPE'),
    ope_i4   INT4_ENC    ENCRYPTED WITH (DEK = localkms_dek, SUPPORT_ALGO = 'OPE'),
    ope_i8   INT8_ENC    ENCRYPTED WITH (DEK = localkms_dek, SUPPORT_ALGO = 'OPE'),
    ope_f4   FLOAT4_ENC  ENCRYPTED WITH (DEK = localkms_dek, SUPPORT_ALGO = 'OPE'),
    ope_f8   FLOAT8_ENC  ENCRYPTED WITH (DEK = localkms_dek, SUPPORT_ALGO = 'OPE'),
    ope_num  NUMERIC_ENC ENCRYPTED WITH (DEK = localkms_dek, SUPPORT_ALGO = 'OPE'),
    ope_vc   VARCHAR_ENC(100) ENCRYPTED WITH (DEK = localkms_dek, SUPPORT_ALGO = 'OPE'),
    ope_txt  TEXT_ENC         ENCRYPTED WITH (DEK = localkms_dek, SUPPORT_ALGO = 'OPE'),
    tag      TEXT
);

-- 表4: SEARCH专项表 — LIKE模糊查询
SELECT '=== Part 2.4: 创建SEARCH专项表 ===' AS stage;

CREATE TABLE localkms_search_tbl (
    id        SERIAL PRIMARY KEY,
    srch_vc   VARCHAR_ENC(200) ENCRYPTED WITH (DEK = localkms_dek, SUPPORT_ALGO = 'DET,SEARCH'),
    srch_bp   BPCHAR_ENC(100)  ENCRYPTED WITH (DEK = localkms_dek, SUPPORT_ALGO = 'DET,SEARCH'),
    srch_txt  TEXT_ENC         ENCRYPTED WITH (DEK = localkms_dek, SUPPORT_ALGO = 'DET,SEARCH'),
    tag       TEXT
);

-- 表5: DEFAULT值测试表
SELECT '=== Part 2.5: 创建DEFAULT值测试表 ===' AS stage;

CREATE TABLE localkms_default_tbl (
    id       INT4 DEFAULT 1,
    def_i4   INT4_ENC    ENCRYPTED WITH (DEK = localkms_dek, SUPPORT_ALGO = 'DET') DEFAULT 100,
    def_vc   VARCHAR_ENC(100) ENCRYPTED WITH (DEK = localkms_dek, SUPPORT_ALGO = 'DET') DEFAULT 'hello_default',
    def_txt  TEXT_ENC    ENCRYPTED WITH (DEK = localkms_dek, SUPPORT_ALGO = 'DET,SEARCH') DEFAULT 'default_text',
    note     TEXT
);

-- ============================================================
-- Part 3: INSERT 数据（多种写法全覆盖）
-- ============================================================

-- ── 3.1 全覆盖表：单行INSERT（完整列） ──
SELECT '=== Part 3.1: 全覆盖表单行INSERT ===' AS stage;

INSERT INTO localkms_all_enc
    (c_int2, c_int4, c_int8, c_f4_det, c_f8_ope, c_num_ope,
     c_vc_det, c_bp_srch, c_txt_all, tag)
VALUES
    (1, 100, 10000, 1.5, 3.14, 999.99,
     'Alice', 'ENGINEER', 'Software Engineer Alice', 'row1');

INSERT INTO localkms_all_enc
    (c_int2, c_int4, c_int8, c_f4_det, c_f8_ope, c_num_ope,
     c_vc_det, c_bp_srch, c_txt_all, tag)
VALUES
    (2, 200, 20000, 2.5, 2.71, 500.50,
     'Bob', 'MANAGER', 'Product Manager Bob', 'row2');

INSERT INTO localkms_all_enc
    (c_int2, c_int4, c_int8, c_f4_det, c_f8_ope, c_num_ope,
     c_vc_det, c_bp_srch, c_txt_all, tag)
VALUES
    (3, 300, 30000, 3.5, 1.41, 0.01,
     'Charlie', 'ANALYST', 'Data Analyst Charlie', 'row3');

-- ── 3.2 全覆盖表：多行INSERT（VALUES列表） ──
SELECT '=== Part 3.2: 全覆盖表多行INSERT ===' AS stage;

INSERT INTO localkms_all_enc
    (c_int2, c_int4, c_int8, c_f4_det, c_f8_ope, c_num_ope,
     c_vc_det, c_bp_srch, c_txt_all, tag)
VALUES
    (10, 1000, 100000, 10.1, 10.01, 111.11, 'Diana',  'DESIGNER', 'UX Designer Diana',  'row4'),
    (20, 2000, 200000, 20.2, 20.02, 222.22, 'Eve',    'DEVOPS',   'DevOps Eve',         'row5'),
    (30, 3000, 300000, 30.3, 30.03, 333.33, 'Frank',  'TESTER',   'QA Tester Frank',    'row6'),
    (40, 4000, 400000, 40.4, 40.04, 444.44, 'Grace',  'ARCHITECT','Architect Grace',    'row7'),
    (50, 5000, 500000, 50.5, 50.05, 555.55, 'Henry',  'DBA',      'Database Admin Henry','row8');

-- ── 3.3 全覆盖表：省略列名的多行INSERT ──
SELECT '=== Part 3.3: 全覆盖表省略列名多行INSERT ===' AS stage;

INSERT INTO localkms_all_enc VALUES
    (DEFAULT, 60, 6000, 600000, 60.6, 60.06, 666.66, 'Ivan',  'SRE',     'SRE Ivan',   'row9'),
    (DEFAULT, 70, 7000, 700000, 70.7, 70.07, 777.77, 'Julia', 'SUPPORT', 'Support Julia','row10');

-- ── 3.4 DET专项表：多行INSERT（混合语法） ──
SELECT '=== Part 3.4: DET专项表INSERT ===' AS stage;

INSERT INTO localkms_det_tbl (det_i2, det_i4, det_i8, det_f4, det_f8, det_num, det_vc, det_bp, det_txt, tag)
VALUES (11, 111, 1111, 11.1, 11.11, 11.111, 'alpha', 'A1', 'DET text alpha', 'det_r1');

INSERT INTO localkms_det_tbl (det_i2, det_i4, det_i8, det_f4, det_f8, det_num, det_vc, det_bp, det_txt, tag)
VALUES (22, 222, 2222, 22.2, 22.22, 22.222, 'beta', 'B2', 'DET text beta', 'det_r2');

INSERT INTO localkms_det_tbl (det_i2, det_i4, det_i8, det_f4, det_f8, det_num, det_vc, det_bp, det_txt, tag)
VALUES
    (33, 333, 3333, 33.3, 33.33, 33.333, 'gamma', 'C3', 'DET text gamma', 'det_r3'),
    (44, 444, 4444, 44.4, 44.44, 44.444, 'alpha', 'D4', 'DET text alpha2', 'det_r4'),
    (55, 555, 5555, 55.5, 55.55, 55.555, 'delta', 'E5', 'DET text delta', 'det_r5');

-- ── 3.5 OPE专项表：多行INSERT（覆盖范围值） ──
SELECT '=== Part 3.5: OPE专项表INSERT ===' AS stage;

INSERT INTO localkms_ope_tbl (ope_i2, ope_i4, ope_i8, ope_f4, ope_f8, ope_num, ope_vc, ope_txt, tag) VALUES
    (-50, -500, -5000, -50.5, -500.5, -555.55, 'AAA', 'ope_minus', 'ope_r1');

INSERT INTO localkms_ope_tbl (ope_i2, ope_i4, ope_i8, ope_f4, ope_f8, ope_num, ope_vc, ope_txt, tag) VALUES
    (0, 0, 0, 0.0, 0.0, 0.0, 'BBB', 'ope_zero', 'ope_r2');

INSERT INTO localkms_ope_tbl (ope_i2, ope_i4, ope_i8, ope_f4, ope_f8, ope_num, ope_vc, ope_txt, tag)
VALUES
    (10, 100, 1000, 10.5, 100.5, 100.55, 'CCC', 'ope_small',  'ope_r3'),
    (20, 200, 2000, 20.5, 200.5, 200.55, 'DDD', 'ope_medium', 'ope_r4'),
    (30, 300, 3000, 30.5, 300.5, 300.55, 'EEE', 'ope_large',  'ope_r5'),
    (40, 400, 4000, 40.5, 400.5, 400.55, 'FFF', 'ope_xlarge', 'ope_r6'),
    (50, 500, 5000, 50.5, 500.5, 500.55, 'GGG', 'ope_max',    'ope_r7');

-- ── 3.6 SEARCH专项表：多行INSERT ──
SELECT '=== Part 3.6: SEARCH专项表INSERT ===' AS stage;

INSERT INTO localkms_search_tbl (srch_vc, srch_bp, srch_txt, tag) VALUES
    ('Alice Wang',    'alice@test.com',   'Alice Wang is a software engineer', 'srch_r1');

INSERT INTO localkms_search_tbl (srch_vc, srch_bp, srch_txt, tag) VALUES
    ('Bob Chen',      'bob@example.org',  'Bob Chen is a product manager',     'srch_r2');

INSERT INTO localkms_search_tbl (srch_vc, srch_bp, srch_txt, tag) VALUES
    ('Charlie Zhang', 'charlie@test.com', 'Charlie Zhang is a data analyst',  'srch_r3');

INSERT INTO localkms_search_tbl (srch_vc, srch_bp, srch_txt, tag) VALUES
    ('David Liu',     'david@demo.net',   'David Liu is a DevOps engineer',   'srch_r4');

INSERT INTO localkms_search_tbl (srch_vc, srch_bp, srch_txt, tag) VALUES
    ('Eve Zhao',      'eve@test.com',     'Eve Zhao is a QA tester',          'srch_r5');

-- ── 3.7 NULL值 / 边界值 INSERT ──
SELECT '=== Part 3.7: NULL和边界值INSERT ===' AS stage;

INSERT INTO localkms_all_enc (c_int4, c_vc_det, c_txt_all, tag)
VALUES (NULL, NULL, NULL, 'null_row');

INSERT INTO localkms_all_enc (c_int4, c_vc_det, c_txt_all, tag)
VALUES (-2147483648, 'min_int', 'INT_MIN boundary test', 'boundary_min');

INSERT INTO localkms_all_enc (c_int4, c_vc_det, c_txt_all, tag)
VALUES (2147483647, 'max_int', 'INT_MAX boundary test', 'boundary_max');

INSERT INTO localkms_all_enc (c_int4, c_vc_det, c_txt_all, tag)
VALUES (-1, 'neg_one', 'Negative one', 'boundary_neg1');

INSERT INTO localkms_all_enc (c_int4, c_vc_det, c_txt_all, tag)
VALUES (0, 'zero', 'Zero value', 'boundary_zero');

INSERT INTO localkms_all_enc (c_int4, c_vc_det, c_txt_all, tag)
VALUES (1, 'one', 'One value', 'boundary_one');

-- ── 3.8 DEFAULT值表INSERT ──
SELECT '=== Part 3.8: DEFAULT值表INSERT ===' AS stage;

INSERT INTO localkms_default_tbl (id, note) VALUES (1, 'all defaults');
INSERT INTO localkms_default_tbl (id, def_i4, def_vc, def_txt, note)
VALUES (2, 200, 'explicit_val', 'explicit_text', 'explicit all');
INSERT INTO localkms_default_tbl (id, def_txt, note)
VALUES (3, 'only_text_set', 'partial default');

-- ============================================================
-- Part 4: SELECT 查询（全场景覆盖）
-- ============================================================

-- ── 4.1 全表解密查询 ──
SELECT '=== Part 4.1: 全表解密 SELECT * ===' AS stage;

SELECT * FROM localkms_all_enc WHERE tag = 'row1';
SELECT id, c_int2, c_int4, c_int8, c_f4_det, c_f8_ope, c_num_ope,
       c_vc_det, c_bp_srch, c_txt_all, tag
FROM localkms_all_enc ORDER BY id;

-- ── 4.2 DET等值查询（所有数值类型 + 字符类型） ──
SELECT '=== Part 4.2: DET等值查询 ===' AS stage;

-- int2 DET等值
SELECT 'DET: int2 = 11' AS test_case;
SELECT tag FROM localkms_det_tbl WHERE det_i2 = 11;

-- int4 DET等值
SELECT 'DET: int4 = 222' AS test_case;
SELECT tag FROM localkms_det_tbl WHERE det_i4 = 222;

-- int8 DET等值
SELECT 'DET: int8 = 3333' AS test_case;
SELECT tag FROM localkms_det_tbl WHERE det_i8 = 3333;

-- float4 DET等值
SELECT 'DET: float4 = 44.4' AS test_case;
SELECT tag FROM localkms_det_tbl WHERE det_f4 = 44.4;

-- float8 DET等值
SELECT 'DET: float8 = 55.55' AS test_case;
SELECT tag FROM localkms_det_tbl WHERE det_f8 = 55.55;

-- numeric DET等值
SELECT 'DET: numeric = 22.222' AS test_case;
SELECT tag FROM localkms_det_tbl WHERE det_num = 22.222;

-- varchar DET等值
SELECT 'DET: varchar = ''alpha''' AS test_case;
SELECT tag FROM localkms_det_tbl WHERE det_vc = 'alpha';

-- bpchar DET等值
SELECT 'DET: bpchar = ''C3''' AS test_case;
SELECT tag FROM localkms_det_tbl WHERE det_bp = 'C3';

-- text DET等值
SELECT 'DET: text = ''DET text delta''' AS test_case;
SELECT tag FROM localkms_det_tbl WHERE det_txt = 'DET text delta';

-- ── 4.3 DET多值匹配（IN） ──
SELECT '=== Part 4.3: DET IN 多值匹配 ===' AS stage;

SELECT 'DET: det_i4 IN (111, 333, 555)' AS test_case;
SELECT tag, det_i4 FROM localkms_det_tbl WHERE det_i4 IN (111, 333, 555) ORDER BY id;

SELECT 'DET: det_vc IN (''alpha'', ''gamma'')' AS test_case;
SELECT tag, det_vc FROM localkms_det_tbl WHERE det_vc IN ('alpha', 'gamma') ORDER BY id;

-- ── 4.4 DET不等值查询（<> / !=） ──
SELECT '=== Part 4.4: DET不等值查询 ===' AS stage;

SELECT 'DET: det_i4 <> 222' AS test_case;
SELECT tag, det_i4 FROM localkms_det_tbl WHERE det_i4 <> 222 ORDER BY id;

SELECT 'DET: det_vc != ''beta''' AS test_case;
SELECT tag, det_vc FROM localkms_det_tbl WHERE det_vc != 'beta' ORDER BY id;

-- ── 4.5 DET AND组合条件 ──
SELECT '=== Part 4.5: DET AND组合条件 ===' AS stage;

SELECT 'DET: det_i4 = 111 AND det_vc = ''alpha''' AS test_case;
SELECT tag FROM localkms_det_tbl WHERE det_i4 = 111 AND det_vc = 'alpha';

SELECT 'DET: det_i2 = 44 AND det_txt = ''DET text alpha2''' AS test_case;
SELECT tag FROM localkms_det_tbl WHERE det_i2 = 44 AND det_txt = 'DET text alpha2';

-- ── 4.6 DET OR条件 ──
SELECT '=== Part 4.6: DET OR条件 ===' AS stage;

SELECT 'DET: det_vc = ''alpha'' OR det_vc = ''gamma''' AS test_case;
SELECT tag, det_vc FROM localkms_det_tbl
WHERE det_vc = 'alpha' OR det_vc = 'gamma' ORDER BY id;

-- ── 4.7 OPE范围查询（> < >= <=） ──
SELECT '=== Part 4.7: OPE范围查询 ===' AS stage;

-- int2 OPE
SELECT 'OPE: ope_i2 > 10' AS test_case;
SELECT ope_i2 FROM localkms_ope_tbl WHERE ope_i2 > 10 ORDER BY ope_i2;

SELECT 'OPE: ope_i2 < 0' AS test_case;
SELECT ope_i2 FROM localkms_ope_tbl WHERE ope_i2 < 0 ORDER BY ope_i2;

SELECT 'OPE: ope_i2 >= 20' AS test_case;
SELECT ope_i2 FROM localkms_ope_tbl WHERE ope_i2 >= 20 ORDER BY ope_i2;

SELECT 'OPE: ope_i2 <= 10' AS test_case;
SELECT ope_i2 FROM localkms_ope_tbl WHERE ope_i2 <= 10 ORDER BY ope_i2;

-- int4 OPE
SELECT 'OPE: ope_i4 > 200' AS test_case;
SELECT ope_i4 FROM localkms_ope_tbl WHERE ope_i4 > 200 ORDER BY ope_i4;

SELECT 'OPE: ope_i4 < 200' AS test_case;
SELECT ope_i4 FROM localkms_ope_tbl WHERE ope_i4 < 200 ORDER BY ope_i4;

-- int8 OPE
SELECT 'OPE: ope_i8 >= 3000' AS test_case;
SELECT ope_i8 FROM localkms_ope_tbl WHERE ope_i8 >= 3000 ORDER BY ope_i8;

SELECT 'OPE: ope_i8 <= 1000' AS test_case;
SELECT ope_i8 FROM localkms_ope_tbl WHERE ope_i8 <= 1000 ORDER BY ope_i8;

-- float4 OPE
SELECT 'OPE: ope_f4 > 20.5' AS test_case;
SELECT ope_f4 FROM localkms_ope_tbl WHERE ope_f4 > 20.5 ORDER BY ope_f4;

-- float8 OPE
SELECT 'OPE: ope_f8 >= 300.5' AS test_case;
SELECT ope_f8 FROM localkms_ope_tbl WHERE ope_f8 >= 300.5 ORDER BY ope_f8;

-- numeric OPE
SELECT 'OPE: ope_num < 200.55' AS test_case;
SELECT ope_num FROM localkms_ope_tbl WHERE ope_num < 200.55 ORDER BY ope_num;

-- varchar OPE（字符串范围比较）
SELECT 'OPE: ope_vc > ''CCC''' AS test_case;
SELECT ope_vc FROM localkms_ope_tbl WHERE ope_vc > 'CCC' ORDER BY ope_vc;

SELECT 'OPE: ope_vc >= ''EEE''' AS test_case;
SELECT ope_vc FROM localkms_ope_tbl WHERE ope_vc >= 'EEE' ORDER BY ope_vc;

-- text OPE
SELECT 'OPE: ope_txt >= ''ope_medium''' AS test_case;
SELECT ope_txt FROM localkms_ope_tbl WHERE ope_txt >= 'ope_medium' ORDER BY ope_txt;

-- ── 4.8 OPE BETWEEN范围查询 ──
SELECT '=== Part 4.8: OPE BETWEEN 范围查询 ===' AS stage;

SELECT 'OPE: ope_i4 BETWEEN 100 AND 400' AS test_case;
SELECT ope_i4 FROM localkms_ope_tbl
WHERE ope_i4 BETWEEN 100 AND 400 ORDER BY ope_i4;

SELECT 'OPE: ope_f8 BETWEEN 100.5 AND 400.5' AS test_case;
SELECT ope_f8 FROM localkms_ope_tbl
WHERE ope_f8 BETWEEN 100.5 AND 400.5 ORDER BY ope_f8;

SELECT 'OPE: ope_vc BETWEEN ''BBB'' AND ''EEE''' AS test_case;
SELECT ope_vc FROM localkms_ope_tbl
WHERE ope_vc BETWEEN 'BBB' AND 'EEE' ORDER BY ope_vc;

-- ── 4.9 OPE ORDER BY（保序加密验证） ──
SELECT '=== Part 4.9: OPE ORDER BY 保序验证 ===' AS stage;

SELECT 'OPE: ORDER BY ope_i4 ASC' AS test_case;
SELECT ope_i4 FROM localkms_ope_tbl ORDER BY ope_i4 ASC;

SELECT 'OPE: ORDER BY ope_i4 DESC' AS test_case;
SELECT ope_i4 FROM localkms_ope_tbl ORDER BY ope_i4 DESC;

SELECT 'OPE: ORDER BY ope_f8 ASC' AS test_case;
SELECT ope_f8 FROM localkms_ope_tbl ORDER BY ope_f8 ASC;

SELECT 'OPE: ORDER BY ope_f8 DESC' AS test_case;
SELECT ope_f8 FROM localkms_ope_tbl ORDER BY ope_f8 DESC;

SELECT 'OPE: ORDER BY ope_num ASC' AS test_case;
SELECT ope_num FROM localkms_ope_tbl ORDER BY ope_num ASC;

SELECT 'OPE: ORDER BY ope_vc ASC' AS test_case;
SELECT ope_vc FROM localkms_ope_tbl ORDER BY ope_vc ASC;

SELECT 'OPE: ORDER BY ope_vc DESC' AS test_case;
SELECT ope_vc FROM localkms_ope_tbl ORDER BY ope_vc DESC;

SELECT 'OPE: ORDER BY ope_txt ASC' AS test_case;
SELECT ope_txt FROM localkms_ope_tbl ORDER BY ope_txt ASC;

-- ── 4.10 OPE范围 + ORDER BY组合 ──
SELECT '=== Part 4.10: OPE范围 + ORDER BY 组合 ===' AS stage;

SELECT 'OPE: ope_i4 > 100 ORDER BY ope_i4 DESC' AS test_case;
SELECT ope_i4 FROM localkms_ope_tbl WHERE ope_i4 > 100 ORDER BY ope_i4 DESC;

SELECT 'OPE: ope_f8 BETWEEN 0 AND 400 ORDER BY ope_f8 ASC' AS test_case;
SELECT ope_f8 FROM localkms_ope_tbl
WHERE ope_f8 BETWEEN 0 AND 400 ORDER BY ope_f8 ASC;

-- ── 4.11 SEARCH LIKE模糊查询 ──
SELECT '=== Part 4.11: SEARCH LIKE 包含匹配 ===' AS stage;

-- LIKE 包含匹配
SELECT 'SEARCH: LIKE ''%eng%''' AS test_case;
SELECT srch_txt FROM localkms_search_tbl WHERE srch_txt LIKE '%eng%';

SELECT 'SEARCH: LIKE ''%manager%''' AS test_case;
SELECT srch_txt FROM localkms_search_tbl WHERE srch_txt LIKE '%manager%';

-- LIKE 前缀匹配
SELECT 'SEARCH: LIKE ''Alice%''' AS test_case;
SELECT srch_vc FROM localkms_search_tbl WHERE srch_vc LIKE 'Alice%';

SELECT 'SEARCH: LIKE ''Bob%''' AS test_case;
SELECT srch_txt FROM localkms_search_tbl WHERE srch_txt LIKE 'Bob%';

-- LIKE 后缀匹配
SELECT 'SEARCH: LIKE ''%test.com''' AS test_case;
SELECT srch_bp FROM localkms_search_tbl WHERE srch_bp LIKE '%test.com';

SELECT 'SEARCH: LIKE ''%engineer''' AS test_case;
SELECT srch_txt FROM localkms_search_tbl WHERE srch_txt LIKE '%engineer';

-- VARCHAR列 LIKE
SELECT 'SEARCH: srch_vc LIKE ''%Zhang%''' AS test_case;
SELECT srch_vc FROM localkms_search_tbl WHERE srch_vc LIKE '%Zhang%';

-- BPCHAR列 LIKE
SELECT 'SEARCH: srch_bp LIKE ''%example%''' AS test_case;
SELECT srch_bp FROM localkms_search_tbl WHERE srch_bp LIKE '%example%';

-- ── 4.12 SEARCH NOT LIKE ──
SELECT '=== Part 4.12: SEARCH NOT LIKE ===' AS stage;

SELECT 'SEARCH: srch_txt NOT LIKE ''%engineer%''' AS test_case;
SELECT srch_txt FROM localkms_search_tbl WHERE srch_txt NOT LIKE '%engineer%';

SELECT 'SEARCH: srch_vc NOT LIKE ''%Zhao%''' AS test_case;
SELECT srch_vc FROM localkms_search_tbl WHERE srch_vc NOT LIKE '%Zhao%';

-- ── 4.13 SEARCH LIKE + AND组合 ──
SELECT '=== Part 4.13: SEARCH LIKE + AND 组合 ===' AS stage;

SELECT 'SEARCH: srch_txt LIKE ''%is a%'' AND srch_bp LIKE ''%test.com%''' AS test_case;
SELECT srch_vc FROM localkms_search_tbl
WHERE srch_txt LIKE '%is a%' AND srch_bp LIKE '%test.com%';

-- ── 4.14 密态列 + 普通列组合条件 ──
SELECT '=== Part 4.14: 密态列 + 普通列 组合条件 ===' AS stage;

SELECT 'MIXED: tag = ''row1'' AND c_vc_det = ''Alice''' AS test_case;
SELECT tag, c_vc_det FROM localkms_all_enc WHERE tag = 'row1' AND c_vc_det = 'Alice';

SELECT 'MIXED: c_f8_ope > 10 AND tag LIKE ''row%''' AS test_case;
SELECT tag, c_f8_ope FROM localkms_all_enc
WHERE c_f8_ope > 10 AND tag LIKE 'row%' ORDER BY id;

-- ── 4.15 NULL值查询 ──
SELECT '=== Part 4.15: NULL值查询 ===' AS stage;

SELECT 'NULL: c_int4 IS NULL' AS test_case;
SELECT tag, c_int4 FROM localkms_all_enc WHERE c_int4 IS NULL;

SELECT 'NULL: c_vc_det IS NOT NULL AND c_int4 IS NULL' AS test_case;
SELECT tag, c_vc_det FROM localkms_all_enc WHERE c_vc_det IS NOT NULL AND c_int4 IS NULL;

-- ── 4.16 边界值查询 ──
SELECT '=== Part 4.16: 边界值查询 ===' AS stage;

SELECT 'BOUNDARY: tag = ''boundary_min''' AS test_case;
SELECT tag, c_int4 FROM localkms_all_enc WHERE tag = 'boundary_min';

SELECT 'BOUNDARY: tag = ''boundary_max''' AS test_case;
SELECT tag, c_int4 FROM localkms_all_enc WHERE tag = 'boundary_max';

SELECT 'BOUNDARY: tag = ''boundary_zero''' AS test_case;
SELECT tag, c_int4 FROM localkms_all_enc WHERE tag = 'boundary_zero';

SELECT 'BOUNDARY: tag = ''boundary_neg1''' AS test_case;
SELECT tag, c_int4 FROM localkms_all_enc WHERE tag = 'boundary_neg1';

-- ── 4.17 DEFAULT值验证 ──
SELECT '=== Part 4.17: DEFAULT值验证 ===' AS stage;

SELECT 'DEFAULT: 全部默认值' AS test_case;
SELECT id, def_i4, def_vc, def_txt, note FROM localkms_default_tbl WHERE id = 1;

SELECT 'DEFAULT: 全部显式值' AS test_case;
SELECT id, def_i4, def_vc, def_txt, note FROM localkms_default_tbl WHERE id = 2;

SELECT 'DEFAULT: 部分默认' AS test_case;
SELECT id, def_i4, def_vc, def_txt, note FROM localkms_default_tbl WHERE id = 3;

SELECT 'DEFAULT: 等值查询 def_i4 = 100' AS test_case;
SELECT id, def_i4 FROM localkms_default_tbl WHERE def_i4 = 100;

SELECT 'DEFAULT: LIKE查询 def_txt LIKE ''%default%''' AS test_case;
SELECT id, def_txt FROM localkms_default_tbl WHERE def_txt LIKE '%default%';

-- ── 4.18 COUNT / 聚合函数查询 ──
SELECT '=== Part 4.18: 聚合函数查询 ===' AS stage;

SELECT 'AGG: COUNT(*)' AS test_case;
SELECT count(*) AS total_rows FROM localkms_all_enc;

SELECT 'AGG: COUNT(c_int4)' AS test_case;
SELECT count(c_int4) AS non_null_count FROM localkms_all_enc;

SELECT 'AGG: MIN/MAX OPE' AS test_case;
SELECT min(ope_i4) AS min_val, max(ope_i4) AS max_val FROM localkms_ope_tbl;

-- ── 4.19 DET自连接（JOIN on encrypted column） ──
SELECT '=== Part 4.19: DET JOIN ===' AS stage;

SELECT 'DET JOIN: a.det_vc = b.det_vc' AS test_case;
SELECT a.tag, b.tag
FROM localkms_det_tbl a
INNER JOIN localkms_det_tbl b ON a.det_vc = b.det_vc
WHERE a.tag = 'det_r1';

-- ── 4.20 子查询 ──
SELECT '=== Part 4.20: 子查询 ===' AS stage;

SELECT 'SUBQUERY: WHERE c_vc_det IN (SELECT det_vc FROM localkms_det_tbl WHERE det_i4 = 111)' AS test_case;
SELECT tag, c_vc_det FROM localkms_all_enc
WHERE c_vc_det IN (SELECT det_vc FROM localkms_det_tbl WHERE det_i4 = 111);

-- ============================================================
-- Part 5: UPDATE 操作
-- ============================================================

-- ── 5.1 按普通列条件UPDATE密态列 ──
SELECT '=== Part 5.1: UPDATE by plaintext condition ===' AS stage;

UPDATE localkms_all_enc SET c_vc_det = 'Alice_Updated' WHERE tag = 'row1';
SELECT tag, c_vc_det FROM localkms_all_enc WHERE tag = 'row1';

-- ── 5.2 按DET密态列条件UPDATE ──
SELECT '=== Part 5.2: UPDATE by DET condition ===' AS stage;

UPDATE localkms_det_tbl SET det_vc = 'alpha_modified' WHERE det_i4 = 111;
SELECT tag, det_vc FROM localkms_det_tbl WHERE tag = 'det_r1';

-- ── 5.3 按OPE范围条件UPDATE ──
SELECT '=== Part 5.3: UPDATE by OPE range ===' AS stage;

UPDATE localkms_ope_tbl SET tag = 'ope_updated' WHERE ope_i4 > 300;
SELECT ope_i4, tag FROM localkms_ope_tbl WHERE ope_i4 > 300 ORDER BY ope_i4;

-- ── 5.4 按SEARCH LIKE条件UPDATE ──
SELECT '=== Part 5.4: UPDATE by SEARCH LIKE ===' AS stage;

UPDATE localkms_search_tbl SET tag = 'srch_updated' WHERE srch_txt LIKE '%engineer%';
SELECT srch_vc, tag FROM localkms_search_tbl WHERE srch_txt LIKE '%engineer%';

-- ── 5.5 UPDATE多列 ──
SELECT '=== Part 5.5: UPDATE 多列 ===' AS stage;

UPDATE localkms_all_enc
SET c_int4 = 99999, c_f8_ope = 99.99, c_txt_all = 'multi_update_test'
WHERE tag = 'row5';
SELECT tag, c_int4, c_f8_ope, c_txt_all FROM localkms_all_enc WHERE tag = 'row5';

-- ============================================================
-- Part 6: DELETE 操作
-- ============================================================

-- ── 6.1 按普通列条件DELETE ──
SELECT '=== Part 6.1: DELETE by plaintext ===' AS stage;

INSERT INTO localkms_all_enc (c_int4, tag) VALUES (88888, 'to_delete_1');
DELETE FROM localkms_all_enc WHERE tag = 'to_delete_1';
SELECT 'DELETE plaintext: row deleted, count check' AS test_case;
SELECT count(*) FROM localkms_all_enc WHERE tag = 'to_delete_1';

-- ── 6.2 按DET密态列等值DELETE ──
SELECT '=== Part 6.2: DELETE by DET ===' AS stage;

INSERT INTO localkms_det_tbl (det_i4, det_vc, tag) VALUES (9999, 'to_delete', 'del_by_det');
DELETE FROM localkms_det_tbl WHERE det_vc = 'to_delete';
SELECT 'DELETE DET: row deleted, count check' AS test_case;
SELECT count(*) FROM localkms_det_tbl WHERE tag = 'del_by_det';

-- ── 6.3 按OPE范围DELETE ──
SELECT '=== Part 6.3: DELETE by OPE range ===' AS stage;

INSERT INTO localkms_ope_tbl (ope_i4, tag) VALUES (999, 'del_ope');
DELETE FROM localkms_ope_tbl WHERE ope_i4 > 500;
SELECT 'DELETE OPE: rows > 500 deleted' AS test_case;
SELECT count(*) FROM localkms_ope_tbl WHERE ope_i4 > 500;

-- ── 6.4 按SEARCH LIKE DELETE ──
SELECT '=== Part 6.4: DELETE by SEARCH LIKE ===' AS stage;

INSERT INTO localkms_search_tbl (srch_txt, tag) VALUES ('This row will be deleted via LIKE', 'del_like');
DELETE FROM localkms_search_tbl WHERE srch_txt LIKE '%deleted via LIKE%';
SELECT 'DELETE SEARCH: row deleted by LIKE' AS test_case;
SELECT count(*) FROM localkms_search_tbl WHERE tag = 'del_like';

-- ============================================================
-- Part 7: 最终验证
-- ============================================================

SELECT '=== Part 7: 最终全表验证 ===' AS stage;

SELECT 'All enc table final state' AS test_case;
SELECT id, tag FROM localkms_all_enc ORDER BY id;

SELECT 'DET table final state' AS test_case;
SELECT id, tag, det_vc FROM localkms_det_tbl ORDER BY id;

SELECT 'OPE table final state' AS test_case;
SELECT id, ope_i4, tag FROM localkms_ope_tbl ORDER BY id;

SELECT 'SEARCH table final state' AS test_case;
SELECT id, srch_vc, tag FROM localkms_search_tbl ORDER BY id;

SELECT 'DEFAULT table final state' AS test_case;
SELECT id, def_i4, def_vc, def_txt, note FROM localkms_default_tbl ORDER BY id;

-- ============================================================
-- Part 8: 清理
-- ============================================================

SELECT '=== Part 8: 清理 ===' AS stage;

DROP TABLE IF EXISTS localkms_all_enc CASCADE;
DROP TABLE IF EXISTS localkms_det_tbl CASCADE;
DROP TABLE IF EXISTS localkms_ope_tbl CASCADE;
DROP TABLE IF EXISTS localkms_search_tbl CASCADE;
DROP TABLE IF EXISTS localkms_default_tbl CASCADE;
DROP DEK IF EXISTS localkms_dek;
DROP KEK IF EXISTS localkms_kek;
