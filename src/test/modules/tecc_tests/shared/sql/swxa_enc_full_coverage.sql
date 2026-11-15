-- ============================================================
-- TECC 硬加密全覆盖用例（SWXA swxakms）
-- 覆盖9种加密类型、多种算法（RND/DET/OPE）
-- 包含多行INSERT（多种写法）、全场景WHERE查询、UPDATE、DELETE
-- 前置：swxa 密码机可达，或 TECC_SDF_LIB 指向 tecc_kms/bin/libteccsdf.so
--       （SoftSDF 模式）；KEK index 复用框架预置的 kek-1（hsm:id=1）
--
-- 放 shared/：本用例只用普通 SQL（无 psql 元命令、无美元引用、注释与字符串内无分号），
-- 两个驱动都能执行 —— libpq 侧走 psql（001），JDBC 侧走 TeccSqlTestRunner（002，
-- 经 JCE 门面 libteccjce.jar）。原在 jdbc/sql/test_swxa_enc_full_coverage.sql：该目录
-- 的用例其实由 psql 执行（run_sql_dir_diff → run_case_sql_via_psql），Java 驱动从未
-- 参与，目录名与归类失真。属 HSM 用例，无密码机时由门禁改名 .skip。
-- ============================================================

-- ============================================================
-- Part 1: 环境准备（SWXA KEK + DEK）
-- ============================================================
DROP SCHEMA IF EXISTS swxa_full_test CASCADE;
DROP DEK IF EXISTS swxa_full_dek CASCADE;
DROP KEK IF EXISTS swxa_full_kek CASCADE;

CREATE SCHEMA IF NOT EXISTS swxa_full_test;

CREATE KEK swxa_full_kek WITH (
    key_store = 'swxakms',
    key_path = 'hsm:id=1'
);

CREATE DEK swxa_full_dek WITH VALUES(
    KEK = swxa_full_kek
);

-- ============================================================
-- Part 2: 建表（覆盖所有加密类型和算法）
-- ============================================================

-- 表1: 全覆盖表 — 9种加密类型，混合算法（RND/DET/OPE/SEARCH）
SELECT '=== Part 2.1: 创建全覆盖表（HSM） ===' AS stage;

CREATE TABLE swxa_full_test.enc_all (
    id        SERIAL PRIMARY KEY,
    -- 数值类型 — DET（仅解密，不做WHERE查询）
    c_int2    INT2_ENC    ENCRYPTED WITH (DEK = swxa_full_dek, SUPPORT_ALGO = 'DET'),
    c_int4    INT4_ENC    ENCRYPTED WITH (DEK = swxa_full_dek, SUPPORT_ALGO = 'DET'),
    c_int8    INT8_ENC    ENCRYPTED WITH (DEK = swxa_full_dek, SUPPORT_ALGO = 'DET'),
    -- 数值类型 — DET + OPE
    c_f4_det  FLOAT4_ENC  ENCRYPTED WITH (DEK = swxa_full_dek, SUPPORT_ALGO = 'DET'),
    c_f8_ope  FLOAT8_ENC  ENCRYPTED WITH (DEK = swxa_full_dek, SUPPORT_ALGO = 'OPE'),
    c_num_ope NUMERIC_ENC ENCRYPTED WITH (DEK = swxa_full_dek, SUPPORT_ALGO = 'OPE'),
    -- 字符类型 — DET / SEARCH
    c_vc_det  VARCHAR_ENC(100) ENCRYPTED WITH (DEK = swxa_full_dek, SUPPORT_ALGO = 'DET'),
    c_bp_srch BPCHAR_ENC(50)   ENCRYPTED WITH (DEK = swxa_full_dek, SUPPORT_ALGO = 'DET,SEARCH'),
    c_txt_all TEXT_ENC         ENCRYPTED WITH (DEK = swxa_full_dek, SUPPORT_ALGO = 'OPE,SEARCH'),
    -- 标记列
    tag       TEXT
);

-- 表2: DET专项表 — 等值查询 + JOIN
SELECT '=== Part 2.2: 创建DET专项表（HSM） ===' AS stage;

CREATE TABLE swxa_full_test.enc_det (
    id       SERIAL PRIMARY KEY,
    det_i2   INT2_ENC    ENCRYPTED WITH (DEK = swxa_full_dek, SUPPORT_ALGO = 'DET'),
    det_i4   INT4_ENC    ENCRYPTED WITH (DEK = swxa_full_dek, SUPPORT_ALGO = 'DET'),
    det_i8   INT8_ENC    ENCRYPTED WITH (DEK = swxa_full_dek, SUPPORT_ALGO = 'DET'),
    det_f4   FLOAT4_ENC  ENCRYPTED WITH (DEK = swxa_full_dek, SUPPORT_ALGO = 'DET'),
    det_f8   FLOAT8_ENC  ENCRYPTED WITH (DEK = swxa_full_dek, SUPPORT_ALGO = 'DET'),
    det_num  NUMERIC_ENC ENCRYPTED WITH (DEK = swxa_full_dek, SUPPORT_ALGO = 'DET'),
    det_vc   VARCHAR_ENC(100) ENCRYPTED WITH (DEK = swxa_full_dek, SUPPORT_ALGO = 'DET'),
    det_bp   BPCHAR_ENC(50)   ENCRYPTED WITH (DEK = swxa_full_dek, SUPPORT_ALGO = 'DET'),
    det_txt  TEXT_ENC         ENCRYPTED WITH (DEK = swxa_full_dek, SUPPORT_ALGO = 'DET'),
    tag      TEXT
);

-- 表3: OPE专项表 — 范围查询 + ORDER BY
SELECT '=== Part 2.3: 创建OPE专项表（HSM） ===' AS stage;

CREATE TABLE swxa_full_test.enc_ope (
    id       SERIAL PRIMARY KEY,
    ope_i2   INT2_ENC    ENCRYPTED WITH (DEK = swxa_full_dek, SUPPORT_ALGO = 'OPE'),
    ope_i4   INT4_ENC    ENCRYPTED WITH (DEK = swxa_full_dek, SUPPORT_ALGO = 'OPE'),
    ope_i8   INT8_ENC    ENCRYPTED WITH (DEK = swxa_full_dek, SUPPORT_ALGO = 'OPE'),
    ope_f4   FLOAT4_ENC  ENCRYPTED WITH (DEK = swxa_full_dek, SUPPORT_ALGO = 'OPE'),
    ope_f8   FLOAT8_ENC  ENCRYPTED WITH (DEK = swxa_full_dek, SUPPORT_ALGO = 'OPE'),
    ope_num  NUMERIC_ENC ENCRYPTED WITH (DEK = swxa_full_dek, SUPPORT_ALGO = 'OPE'),
    ope_vc   VARCHAR_ENC(100) ENCRYPTED WITH (DEK = swxa_full_dek, SUPPORT_ALGO = 'OPE'),
    ope_txt  TEXT_ENC         ENCRYPTED WITH (DEK = swxa_full_dek, SUPPORT_ALGO = 'OPE'),
    tag      TEXT
);

-- 表4: SEARCH专项表 — LIKE模糊查询
SELECT '=== Part 2.4: 创建SEARCH专项表（HSM） ===' AS stage;

CREATE TABLE swxa_full_test.enc_search (
    id        SERIAL PRIMARY KEY,
    srch_vc   VARCHAR_ENC(200) ENCRYPTED WITH (DEK = swxa_full_dek, SUPPORT_ALGO = 'DET,SEARCH'),
    srch_bp   BPCHAR_ENC(100)  ENCRYPTED WITH (DEK = swxa_full_dek, SUPPORT_ALGO = 'DET,SEARCH'),
    srch_txt  TEXT_ENC         ENCRYPTED WITH (DEK = swxa_full_dek, SUPPORT_ALGO = 'DET,SEARCH'),
    tag       TEXT
);

-- 表5: DEFAULT值测试表
SELECT '=== Part 2.5: 创建DEFAULT值测试表（HSM） ===' AS stage;

CREATE TABLE swxa_full_test.enc_default (
    id       INT4 DEFAULT 1,
    def_i4   INT4_ENC    ENCRYPTED WITH (DEK = swxa_full_dek, SUPPORT_ALGO = 'DET') DEFAULT 100,
    def_vc   VARCHAR_ENC(100) ENCRYPTED WITH (DEK = swxa_full_dek, SUPPORT_ALGO = 'DET') DEFAULT 'hsm_default_hello',
    def_txt  TEXT_ENC    ENCRYPTED WITH (DEK = swxa_full_dek, SUPPORT_ALGO = 'DET,SEARCH') DEFAULT 'hsm_default_text',
    note     TEXT
);

-- ============================================================
-- Part 3: INSERT 数据（多种写法全覆盖）
-- ============================================================

-- ── 3.1 全覆盖表：单行INSERT（显式列名） ──
SELECT '=== Part 3.1: 全覆盖表单行INSERT（HSM） ===' AS stage;

INSERT INTO swxa_full_test.enc_all
    (c_int2, c_int4, c_int8, c_f4_det, c_f8_ope, c_num_ope,
     c_vc_det, c_bp_srch, c_txt_all, tag)
VALUES
    (1, 100, 10000, 1.5, 3.14159, 9999.99,
     'Hello_HSM', 'ENGINEER', 'HSM Software Engineer', 'hsm_row1');

INSERT INTO swxa_full_test.enc_all
    (c_int2, c_int4, c_int8, c_f4_det, c_f8_ope, c_num_ope,
     c_vc_det, c_bp_srch, c_txt_all, tag)
VALUES
    (2, 200, 20000, 2.5, 2.71828, 500.50,
     'World_HSM', 'MANAGER', 'HSM Product Manager', 'hsm_row2');

INSERT INTO swxa_full_test.enc_all
    (c_int2, c_int4, c_int8, c_f4_det, c_f8_ope, c_num_ope,
     c_vc_det, c_bp_srch, c_txt_all, tag)
VALUES
    (3, 300, 30000, 3.5, 1.41421, 0.01,
     'Test_HSM', 'ANALYST', 'HSM Data Analyst', 'hsm_row3');

-- ── 3.2 全覆盖表：多行INSERT（VALUES多行列表） ──
SELECT '=== Part 3.2: 全覆盖表多行INSERT（HSM） ===' AS stage;

INSERT INTO swxa_full_test.enc_all
    (c_int2, c_int4, c_int8, c_f4_det, c_f8_ope, c_num_ope,
     c_vc_det, c_bp_srch, c_txt_all, tag)
VALUES
    (10, 1000, 100000, 10.10, 10.01, 111.11,
     'Diana_HSM',  'DESIGNER',  'HSM UX Designer Diana',  'hsm_row4'),
    (20, 2000, 200000, 20.20, 20.02, 222.22,
     'Eve_HSM',    'DEVOPS',    'HSM DevOps Eve',         'hsm_row5'),
    (30, 3000, 300000, 30.30, 30.03, 333.33,
     'Frank_HSM',  'TESTER',    'HSM QA Tester Frank',    'hsm_row6'),
    (40, 4000, 400000, 40.40, 40.04, 444.44,
     'Grace_HSM',  'ARCHITECT', 'HSM Architect Grace',    'hsm_row7'),
    (50, 5000, 500000, 50.50, 50.05, 555.55,
     'Henry_HSM',  'DBA',       'HSM Database Admin Henry','hsm_row8');

-- ── 3.3 全覆盖表：省略列名的多行INSERT ──
SELECT '=== Part 3.3: 全覆盖表省略列名多行INSERT（HSM） ===' AS stage;

INSERT INTO swxa_full_test.enc_all VALUES
    (DEFAULT, 60, 6000, 600000, 60.6, 60.06, 666.66,
     'Ivan_HSM',  'SRE',     'HSM SRE Ivan',   'hsm_row9'),
    (DEFAULT, 70, 7000, 700000, 70.7, 70.07, 777.77,
     'Julia_HSM', 'SUPPORT', 'HSM Support Julia', 'hsm_row10');

-- ── 3.4 DET专项表：INSERT（混合单行/多行语法） ──
SELECT '=== Part 3.4: DET专项表INSERT（HSM） ===' AS stage;

INSERT INTO swxa_full_test.enc_det
    (det_i2, det_i4, det_i8, det_f4, det_f8, det_num, det_vc, det_bp, det_txt, tag)
VALUES
    (11, 111, 1111, 11.1, 11.11, 11.111, 'alpha_hsm', 'A1', 'HSM DET text alpha', 'hsm_det_r1');

INSERT INTO swxa_full_test.enc_det
    (det_i2, det_i4, det_i8, det_f4, det_f8, det_num, det_vc, det_bp, det_txt, tag)
VALUES
    (22, 222, 2222, 22.2, 22.22, 22.222, 'beta_hsm', 'B2', 'HSM DET text beta', 'hsm_det_r2');

INSERT INTO swxa_full_test.enc_det
    (det_i2, det_i4, det_i8, det_f4, det_f8, det_num, det_vc, det_bp, det_txt, tag)
VALUES
    (33, 333, 3333, 33.3, 33.33, 33.333, 'gamma_hsm', 'C3', 'HSM DET text gamma', 'hsm_det_r3'),
    (44, 444, 4444, 44.4, 44.44, 44.444, 'alpha_hsm', 'D4', 'HSM DET text alpha2', 'hsm_det_r4'),
    (55, 555, 5555, 55.5, 55.55, 55.555, 'delta_hsm', 'E5', 'HSM DET text delta', 'hsm_det_r5');

-- ── 3.5 OPE专项表：INSERT（覆盖负数、零、正数范围） ──
SELECT '=== Part 3.5: OPE专项表INSERT（HSM） ===' AS stage;

INSERT INTO swxa_full_test.enc_ope
    (ope_i2, ope_i4, ope_i8, ope_f4, ope_f8, ope_num, ope_vc, ope_txt, tag)
VALUES
    (-50, -500, -5000, -50.5, -500.5, -555.55, 'AAA', 'hsm_ope_minus', 'hsm_ope_r1');

INSERT INTO swxa_full_test.enc_ope
    (ope_i2, ope_i4, ope_i8, ope_f4, ope_f8, ope_num, ope_vc, ope_txt, tag)
VALUES
    (0, 0, 0, 0.0, 0.0, 0.0, 'BBB', 'hsm_ope_zero', 'hsm_ope_r2');

INSERT INTO swxa_full_test.enc_ope
    (ope_i2, ope_i4, ope_i8, ope_f4, ope_f8, ope_num, ope_vc, ope_txt, tag)
VALUES
    (10, 100, 1000, 10.5, 100.5, 100.55, 'CCC', 'hsm_ope_small',  'hsm_ope_r3'),
    (20, 200, 2000, 20.5, 200.5, 200.55, 'DDD', 'hsm_ope_medium', 'hsm_ope_r4'),
    (30, 300, 3000, 30.5, 300.5, 300.55, 'EEE', 'hsm_ope_large',  'hsm_ope_r5'),
    (40, 400, 4000, 40.5, 400.5, 400.55, 'FFF', 'hsm_ope_xlarge', 'hsm_ope_r6'),
    (50, 500, 5000, 50.5, 500.5, 500.55, 'GGG', 'hsm_ope_max',    'hsm_ope_r7');

-- ── 3.6 SEARCH专项表：INSERT ──
SELECT '=== Part 3.6: SEARCH专项表INSERT（HSM） ===' AS stage;

INSERT INTO swxa_full_test.enc_search
    (srch_vc, srch_bp, srch_txt, tag)
VALUES
    ('Alice HSM Wang',  'alice@hsm.com',   'Alice Wang is a HSM software engineer', 'hsm_srch_r1');

INSERT INTO swxa_full_test.enc_search
    (srch_vc, srch_bp, srch_txt, tag)
VALUES
    ('Bob HSM Chen',    'bob@hsm.org',     'Bob Chen is a HSM product manager',     'hsm_srch_r2');

INSERT INTO swxa_full_test.enc_search
    (srch_vc, srch_bp, srch_txt, tag)
VALUES
    ('Charlie HSM Zhang','charlie@hsm.com','Charlie Zhang is a HSM data analyst',  'hsm_srch_r3');

INSERT INTO swxa_full_test.enc_search
    (srch_vc, srch_bp, srch_txt, tag)
VALUES
    ('David HSM Liu',   'david@hsm.net',   'David Liu is a HSM DevOps engineer',   'hsm_srch_r4');

INSERT INTO swxa_full_test.enc_search
    (srch_vc, srch_bp, srch_txt, tag)
VALUES
    ('Eve HSM Zhao',    'eve@hsm.com',     'Eve Zhao is a HSM QA tester',          'hsm_srch_r5');

-- ── 3.7 NULL值 / 边界值 INSERT ──
SELECT '=== Part 3.7: NULL和边界值INSERT（HSM） ===' AS stage;

INSERT INTO swxa_full_test.enc_all (c_int4, c_vc_det, c_txt_all, tag)
VALUES (NULL, NULL, NULL, 'hsm_null_row');

INSERT INTO swxa_full_test.enc_all (c_int4, c_vc_det, c_txt_all, tag)
VALUES (-2147483648, 'hsm_min_int', 'HSM INT_MIN boundary test', 'hsm_boundary_min');

INSERT INTO swxa_full_test.enc_all (c_int4, c_vc_det, c_txt_all, tag)
VALUES (2147483647, 'hsm_max_int', 'HSM INT_MAX boundary test', 'hsm_boundary_max');

INSERT INTO swxa_full_test.enc_all (c_int4, c_vc_det, c_txt_all, tag)
VALUES (-1, 'hsm_neg_one', 'HSM Negative one', 'hsm_boundary_neg1');

INSERT INTO swxa_full_test.enc_all (c_int4, c_vc_det, c_txt_all, tag)
VALUES (0, 'hsm_zero', 'HSM Zero value', 'hsm_boundary_zero');

INSERT INTO swxa_full_test.enc_all (c_int4, c_vc_det, c_txt_all, tag)
VALUES (1, 'hsm_one', 'HSM One value', 'hsm_boundary_one');

-- ── 3.8 DEFAULT值表INSERT ──
SELECT '=== Part 3.8: DEFAULT值表INSERT（HSM） ===' AS stage;

INSERT INTO swxa_full_test.enc_default (id, note) VALUES (1, 'hsm all defaults');
INSERT INTO swxa_full_test.enc_default (id, def_i4, def_vc, def_txt, note)
VALUES (2, 200, 'hsm_explicit_val', 'hsm_explicit_text', 'hsm explicit all');
INSERT INTO swxa_full_test.enc_default (id, def_txt, note)
VALUES (3, 'hsm_only_text_set', 'hsm partial default');

-- ============================================================
-- Part 4: SELECT 查询（全场景覆盖，HSM解密路径）
-- ============================================================

-- ── 4.1 全表解密验证 ──
SELECT '=== Part 4.1: 全表解密 SELECT *（HSM） ===' AS stage;

SELECT * FROM swxa_full_test.enc_all WHERE tag = 'hsm_row1';

SELECT id, c_int2, c_int4, c_int8, c_f4_det, c_f8_ope, c_num_ope,
       c_vc_det, c_bp_srch, c_txt_all, tag
FROM swxa_full_test.enc_all ORDER BY id;

-- ── 4.2 逐列解密验证 ──
SELECT '=== Part 4.2: 逐列解密验证（HSM） ===' AS stage;

SELECT 'c_int2' AS col, c_int2 FROM swxa_full_test.enc_all WHERE tag = 'hsm_row1';
SELECT 'c_int4' AS col, c_int4 FROM swxa_full_test.enc_all WHERE tag = 'hsm_row1';
SELECT 'c_int8' AS col, c_int8 FROM swxa_full_test.enc_all WHERE tag = 'hsm_row1';
SELECT 'c_f4_det' AS col, c_f4_det FROM swxa_full_test.enc_all WHERE tag = 'hsm_row1';
SELECT 'c_f8_ope' AS col, c_f8_ope FROM swxa_full_test.enc_all WHERE tag = 'hsm_row1';
SELECT 'c_num_ope' AS col, c_num_ope FROM swxa_full_test.enc_all WHERE tag = 'hsm_row1';
SELECT 'c_vc_det' AS col, c_vc_det FROM swxa_full_test.enc_all WHERE tag = 'hsm_row1';
SELECT 'c_bp_srch' AS col, c_bp_srch FROM swxa_full_test.enc_all WHERE tag = 'hsm_row1';
SELECT 'c_txt_all' AS col, c_txt_all FROM swxa_full_test.enc_all WHERE tag = 'hsm_row1';

-- ── 4.3 DET等值查询（所有数值类型 + 字符类型） ──
SELECT '=== Part 4.3: DET等值查询（HSM） ===' AS stage;

-- int2 DET
SELECT 'DET: int2 = 11 (HSM)' AS test_case;
SELECT tag FROM swxa_full_test.enc_det WHERE det_i2 = 11;

-- int4 DET
SELECT 'DET: int4 = 222 (HSM)' AS test_case;
SELECT tag FROM swxa_full_test.enc_det WHERE det_i4 = 222;

-- int8 DET
SELECT 'DET: int8 = 3333 (HSM)' AS test_case;
SELECT tag FROM swxa_full_test.enc_det WHERE det_i8 = 3333;

-- float4 DET
SELECT 'DET: float4 = 44.4 (HSM)' AS test_case;
SELECT tag FROM swxa_full_test.enc_det WHERE det_f4 = 44.4;

-- float8 DET
SELECT 'DET: float8 = 55.55 (HSM)' AS test_case;
SELECT tag FROM swxa_full_test.enc_det WHERE det_f8 = 55.55;

-- numeric DET
SELECT 'DET: numeric = 22.222 (HSM)' AS test_case;
SELECT tag FROM swxa_full_test.enc_det WHERE det_num = 22.222;

-- varchar DET
SELECT 'DET: varchar = ''alpha_hsm'' (HSM)' AS test_case;
SELECT tag FROM swxa_full_test.enc_det WHERE det_vc = 'alpha_hsm';

-- bpchar DET
SELECT 'DET: bpchar = ''C3'' (HSM)' AS test_case;
SELECT tag FROM swxa_full_test.enc_det WHERE det_bp = 'C3';

-- text DET
SELECT 'DET: text = ''HSM DET text delta'' (HSM)' AS test_case;
SELECT tag FROM swxa_full_test.enc_det WHERE det_txt = 'HSM DET text delta';

-- ── 4.4 DET多值匹配（IN） ──
SELECT '=== Part 4.4: DET IN 多值匹配（HSM） ===' AS stage;

SELECT 'DET: det_i4 IN (111, 333, 555) (HSM)' AS test_case;
SELECT tag, det_i4 FROM swxa_full_test.enc_det
WHERE det_i4 IN (111, 333, 555) ORDER BY id;

SELECT 'DET: det_vc IN (''alpha_hsm'', ''gamma_hsm'') (HSM)' AS test_case;
SELECT tag, det_vc FROM swxa_full_test.enc_det
WHERE det_vc IN ('alpha_hsm', 'gamma_hsm') ORDER BY id;

-- ── 4.5 DET不等值查询（<> / !=） ──
SELECT '=== Part 4.5: DET不等值查询（HSM） ===' AS stage;

SELECT 'DET: det_i4 <> 222 (HSM)' AS test_case;
SELECT tag, det_i4 FROM swxa_full_test.enc_det WHERE det_i4 <> 222 ORDER BY id;

SELECT 'DET: det_vc != ''beta_hsm'' (HSM)' AS test_case;
SELECT tag, det_vc FROM swxa_full_test.enc_det WHERE det_vc != 'beta_hsm' ORDER BY id;

-- ── 4.6 DET AND组合条件 ──
SELECT '=== Part 4.6: DET AND组合条件（HSM） ===' AS stage;

SELECT 'DET: det_i4 = 111 AND det_vc = ''alpha_hsm'' (HSM)' AS test_case;
SELECT tag FROM swxa_full_test.enc_det WHERE det_i4 = 111 AND det_vc = 'alpha_hsm';

SELECT 'DET: det_i2 = 44 AND det_txt = ''HSM DET text alpha2'' (HSM)' AS test_case;
SELECT tag FROM swxa_full_test.enc_det
WHERE det_i2 = 44 AND det_txt = 'HSM DET text alpha2';

-- ── 4.7 DET OR条件 ──
SELECT '=== Part 4.7: DET OR条件（HSM） ===' AS stage;

SELECT 'DET: det_vc = ''alpha_hsm'' OR det_vc = ''gamma_hsm'' (HSM)' AS test_case;
SELECT tag, det_vc FROM swxa_full_test.enc_det
WHERE det_vc = 'alpha_hsm' OR det_vc = 'gamma_hsm' ORDER BY id;

-- ── 4.8 DET JOIN（加密列等值连接） ──
SELECT '=== Part 4.8: DET JOIN（HSM） ===' AS stage;

SELECT 'DET JOIN: a.det_vc = b.det_vc (HSM)' AS test_case;
SELECT a.tag, b.tag
FROM swxa_full_test.enc_det a
INNER JOIN swxa_full_test.enc_det b ON a.det_vc = b.det_vc
WHERE a.tag = 'hsm_det_r1';

-- ── 4.9 OPE范围查询（> < >= <=） ──
SELECT '=== Part 4.9: OPE范围查询（HSM） ===' AS stage;

-- int2 OPE
SELECT 'OPE: ope_i2 > 10 (HSM)' AS test_case;
SELECT ope_i2 FROM swxa_full_test.enc_ope WHERE ope_i2 > 10 ORDER BY ope_i2;

SELECT 'OPE: ope_i2 < 0 (HSM)' AS test_case;
SELECT ope_i2 FROM swxa_full_test.enc_ope WHERE ope_i2 < 0 ORDER BY ope_i2;

SELECT 'OPE: ope_i2 >= 20 (HSM)' AS test_case;
SELECT ope_i2 FROM swxa_full_test.enc_ope WHERE ope_i2 >= 20 ORDER BY ope_i2;

SELECT 'OPE: ope_i2 <= 10 (HSM)' AS test_case;
SELECT ope_i2 FROM swxa_full_test.enc_ope WHERE ope_i2 <= 10 ORDER BY ope_i2;

-- int4 OPE
SELECT 'OPE: ope_i4 > 200 (HSM)' AS test_case;
SELECT ope_i4 FROM swxa_full_test.enc_ope WHERE ope_i4 > 200 ORDER BY ope_i4;

SELECT 'OPE: ope_i4 < 200 (HSM)' AS test_case;
SELECT ope_i4 FROM swxa_full_test.enc_ope WHERE ope_i4 < 200 ORDER BY ope_i4;

-- int8 OPE
SELECT 'OPE: ope_i8 >= 3000 (HSM)' AS test_case;
SELECT ope_i8 FROM swxa_full_test.enc_ope WHERE ope_i8 >= 3000 ORDER BY ope_i8;

SELECT 'OPE: ope_i8 <= 1000 (HSM)' AS test_case;
SELECT ope_i8 FROM swxa_full_test.enc_ope WHERE ope_i8 <= 1000 ORDER BY ope_i8;

-- float4 OPE
SELECT 'OPE: ope_f4 > 20.5 (HSM)' AS test_case;
SELECT ope_f4 FROM swxa_full_test.enc_ope WHERE ope_f4 > 20.5 ORDER BY ope_f4;

-- float8 OPE
SELECT 'OPE: ope_f8 >= 300.5 (HSM)' AS test_case;
SELECT ope_f8 FROM swxa_full_test.enc_ope WHERE ope_f8 >= 300.5 ORDER BY ope_f8;

-- numeric OPE
SELECT 'OPE: ope_num < 200.55 (HSM)' AS test_case;
SELECT ope_num FROM swxa_full_test.enc_ope WHERE ope_num < 200.55 ORDER BY ope_num;

-- varchar OPE（字符串范围比较）
SELECT 'OPE: ope_vc > ''CCC'' (HSM)' AS test_case;
SELECT ope_vc FROM swxa_full_test.enc_ope WHERE ope_vc > 'CCC' ORDER BY ope_vc;

SELECT 'OPE: ope_vc >= ''EEE'' (HSM)' AS test_case;
SELECT ope_vc FROM swxa_full_test.enc_ope WHERE ope_vc >= 'EEE' ORDER BY ope_vc;

-- text OPE
SELECT 'OPE: ope_txt >= ''hsm_ope_medium'' (HSM)' AS test_case;
SELECT ope_txt FROM swxa_full_test.enc_ope
WHERE ope_txt >= 'hsm_ope_medium' ORDER BY ope_txt;

-- ── 4.10 OPE BETWEEN范围查询 ──
SELECT '=== Part 4.10: OPE BETWEEN范围查询（HSM） ===' AS stage;

SELECT 'OPE: ope_i4 BETWEEN 100 AND 400 (HSM)' AS test_case;
SELECT ope_i4 FROM swxa_full_test.enc_ope
WHERE ope_i4 BETWEEN 100 AND 400 ORDER BY ope_i4;

SELECT 'OPE: ope_f8 BETWEEN 100.5 AND 400.5 (HSM)' AS test_case;
SELECT ope_f8 FROM swxa_full_test.enc_ope
WHERE ope_f8 BETWEEN 100.5 AND 400.5 ORDER BY ope_f8;

SELECT 'OPE: ope_num BETWEEN 0 AND 300.55 (HSM)' AS test_case;
SELECT ope_num FROM swxa_full_test.enc_ope
WHERE ope_num BETWEEN 0 AND 300.55 ORDER BY ope_num;

SELECT 'OPE: ope_vc BETWEEN ''BBB'' AND ''EEE'' (HSM)' AS test_case;
SELECT ope_vc FROM swxa_full_test.enc_ope
WHERE ope_vc BETWEEN 'BBB' AND 'EEE' ORDER BY ope_vc;

-- ── 4.11 OPE ORDER BY（保序加密验证） ──
SELECT '=== Part 4.11: OPE ORDER BY 保序验证（HSM） ===' AS stage;

SELECT 'OPE: ORDER BY ope_i4 ASC (HSM)' AS test_case;
SELECT ope_i4 FROM swxa_full_test.enc_ope ORDER BY ope_i4 ASC;

SELECT 'OPE: ORDER BY ope_i4 DESC (HSM)' AS test_case;
SELECT ope_i4 FROM swxa_full_test.enc_ope ORDER BY ope_i4 DESC;

SELECT 'OPE: ORDER BY ope_f8 ASC (HSM)' AS test_case;
SELECT ope_f8 FROM swxa_full_test.enc_ope ORDER BY ope_f8 ASC;

SELECT 'OPE: ORDER BY ope_f8 DESC (HSM)' AS test_case;
SELECT ope_f8 FROM swxa_full_test.enc_ope ORDER BY ope_f8 DESC;

SELECT 'OPE: ORDER BY ope_num ASC (HSM)' AS test_case;
SELECT ope_num FROM swxa_full_test.enc_ope ORDER BY ope_num ASC;

SELECT 'OPE: ORDER BY ope_vc ASC (HSM)' AS test_case;
SELECT ope_vc FROM swxa_full_test.enc_ope ORDER BY ope_vc ASC;

SELECT 'OPE: ORDER BY ope_vc DESC (HSM)' AS test_case;
SELECT ope_vc FROM swxa_full_test.enc_ope ORDER BY ope_vc DESC;

SELECT 'OPE: ORDER BY ope_txt ASC (HSM)' AS test_case;
SELECT ope_txt FROM swxa_full_test.enc_ope ORDER BY ope_txt ASC;

-- ── 4.12 OPE范围 + ORDER BY组合 ──
SELECT '=== Part 4.12: OPE范围 + ORDER BY 组合（HSM） ===' AS stage;

SELECT 'OPE: ope_i4 > 100 ORDER BY ope_i4 DESC (HSM)' AS test_case;
SELECT ope_i4 FROM swxa_full_test.enc_ope WHERE ope_i4 > 100 ORDER BY ope_i4 DESC;

SELECT 'OPE: ope_f8 BETWEEN 0 AND 400 ORDER BY ope_f8 ASC (HSM)' AS test_case;
SELECT ope_f8 FROM swxa_full_test.enc_ope
WHERE ope_f8 BETWEEN 0 AND 400 ORDER BY ope_f8 ASC;

-- ── 4.13 SEARCH LIKE模糊查询 ──
SELECT '=== Part 4.13: SEARCH LIKE 包含匹配（HSM） ===' AS stage;

-- LIKE 包含匹配
SELECT 'SEARCH: LIKE ''%HSM%'' (HSM)' AS test_case;
SELECT srch_txt FROM swxa_full_test.enc_search WHERE srch_txt LIKE '%HSM%';

SELECT 'SEARCH: LIKE ''%engineer%'' (HSM)' AS test_case;
SELECT srch_txt FROM swxa_full_test.enc_search WHERE srch_txt LIKE '%engineer%';

SELECT 'SEARCH: LIKE ''%manager%'' (HSM)' AS test_case;
SELECT srch_txt FROM swxa_full_test.enc_search WHERE srch_txt LIKE '%manager%';

-- LIKE 前缀匹配
SELECT 'SEARCH: LIKE ''Alice%'' (HSM)' AS test_case;
SELECT srch_vc FROM swxa_full_test.enc_search WHERE srch_vc LIKE 'Alice%';

SELECT 'SEARCH: LIKE ''Bob%'' (HSM)' AS test_case;
SELECT srch_txt FROM swxa_full_test.enc_search WHERE srch_txt LIKE 'Bob%';

-- LIKE 后缀匹配
SELECT 'SEARCH: LIKE ''%hsm.com'' (HSM)' AS test_case;
SELECT srch_bp FROM swxa_full_test.enc_search WHERE srch_bp LIKE '%hsm.com';

SELECT 'SEARCH: LIKE ''%engineer'' (HSM)' AS test_case;
SELECT srch_txt FROM swxa_full_test.enc_search WHERE srch_txt LIKE '%engineer';

-- VARCHAR列 LIKE
SELECT 'SEARCH: srch_vc LIKE ''%Zhang%'' (HSM)' AS test_case;
SELECT srch_vc FROM swxa_full_test.enc_search WHERE srch_vc LIKE '%Zhang%';

-- BPCHAR列 LIKE
SELECT 'SEARCH: srch_bp LIKE ''%hsm.org%'' (HSM)' AS test_case;
SELECT srch_bp FROM swxa_full_test.enc_search WHERE srch_bp LIKE '%hsm.org%';

-- ── 4.14 SEARCH NOT LIKE ──
SELECT '=== Part 4.14: SEARCH NOT LIKE（HSM） ===' AS stage;

SELECT 'SEARCH: srch_txt NOT LIKE ''%engineer%'' (HSM)' AS test_case;
SELECT srch_txt FROM swxa_full_test.enc_search WHERE srch_txt NOT LIKE '%engineer%';

SELECT 'SEARCH: srch_vc NOT LIKE ''%Zhao%'' (HSM)' AS test_case;
SELECT srch_vc FROM swxa_full_test.enc_search WHERE srch_vc NOT LIKE '%Zhao%';

-- ── 4.15 SEARCH LIKE + AND组合 ──
SELECT '=== Part 4.15: SEARCH LIKE + AND 组合（HSM） ===' AS stage;

SELECT 'SEARCH: srch_txt LIKE ''%HSM%'' AND srch_bp LIKE ''%hsm.com%'' (HSM)' AS test_case;
SELECT srch_vc FROM swxa_full_test.enc_search
WHERE srch_txt LIKE '%HSM%' AND srch_bp LIKE '%hsm.com%';

-- ── 4.16 密态列 + 普通列组合条件 ──
SELECT '=== Part 4.16: 密态列 + 普通列 组合条件（HSM） ===' AS stage;

SELECT 'MIXED: tag = ''hsm_row1'' AND c_vc_det = ''Hello_HSM'' (HSM)' AS test_case;
SELECT tag, c_vc_det FROM swxa_full_test.enc_all
WHERE tag = 'hsm_row1' AND c_vc_det = 'Hello_HSM';

SELECT 'MIXED: c_f8_ope > 10 AND tag LIKE ''hsm_row%'' (HSM)' AS test_case;
SELECT tag, c_f8_ope FROM swxa_full_test.enc_all
WHERE c_f8_ope > 10 AND tag LIKE 'hsm_row%' ORDER BY id;

-- ── 4.17 NULL值查询 ──
SELECT '=== Part 4.17: NULL值查询（HSM） ===' AS stage;

SELECT 'NULL: c_int4 IS NULL (HSM)' AS test_case;
SELECT tag, c_int4 FROM swxa_full_test.enc_all WHERE c_int4 IS NULL;

SELECT 'NULL: c_vc_det IS NOT NULL AND c_int4 IS NULL (HSM)' AS test_case;
SELECT tag, c_vc_det FROM swxa_full_test.enc_all
WHERE c_vc_det IS NOT NULL AND c_int4 IS NULL;

-- ── 4.18 边界值查询 ──
SELECT '=== Part 4.18: 边界值查询（HSM） ===' AS stage;

SELECT 'BOUNDARY: tag = ''hsm_boundary_min'' (HSM)' AS test_case;
SELECT tag, c_int4 FROM swxa_full_test.enc_all WHERE tag = 'hsm_boundary_min';

SELECT 'BOUNDARY: tag = ''hsm_boundary_max'' (HSM)' AS test_case;
SELECT tag, c_int4 FROM swxa_full_test.enc_all WHERE tag = 'hsm_boundary_max';

SELECT 'BOUNDARY: tag = ''hsm_boundary_zero'' (HSM)' AS test_case;
SELECT tag, c_int4 FROM swxa_full_test.enc_all WHERE tag = 'hsm_boundary_zero';

SELECT 'BOUNDARY: tag = ''hsm_boundary_neg1'' (HSM)' AS test_case;
SELECT tag, c_int4 FROM swxa_full_test.enc_all WHERE tag = 'hsm_boundary_neg1';

-- ── 4.19 DEFAULT值验证 ──
SELECT '=== Part 4.19: DEFAULT值验证（HSM） ===' AS stage;

SELECT 'DEFAULT: 全部默认值 (HSM)' AS test_case;
SELECT id, def_i4, def_vc, def_txt, note FROM swxa_full_test.enc_default WHERE id = 1;

SELECT 'DEFAULT: 全部显式值 (HSM)' AS test_case;
SELECT id, def_i4, def_vc, def_txt, note FROM swxa_full_test.enc_default WHERE id = 2;

SELECT 'DEFAULT: 部分默认 (HSM)' AS test_case;
SELECT id, def_i4, def_vc, def_txt, note FROM swxa_full_test.enc_default WHERE id = 3;

SELECT 'DEFAULT: 等值查询 def_i4 = 100 (HSM)' AS test_case;
SELECT id, def_i4 FROM swxa_full_test.enc_default WHERE def_i4 = 100;

SELECT 'DEFAULT: LIKE查询 def_txt LIKE ''%hsm%'' (HSM)' AS test_case;
SELECT id, def_txt FROM swxa_full_test.enc_default WHERE def_txt LIKE '%hsm%';

-- ── 4.20 聚合函数查询 ──
SELECT '=== Part 4.20: 聚合函数查询（HSM） ===' AS stage;

SELECT 'AGG: COUNT(*) (HSM)' AS test_case;
SELECT count(*) AS total_rows FROM swxa_full_test.enc_all;

SELECT 'AGG: COUNT(c_int4) (HSM)' AS test_case;
SELECT count(c_int4) AS non_null_count FROM swxa_full_test.enc_all;

-- 曾已知差异：HSM 下 OPE 列的 min/max 两个驱动都拿不到值 —— libpq 侧正文无结果，报错在
-- stderr（`TECC result processing error: Batch decrypt: item 0 HSM not available`，整段
-- 追加在输出末尾，故正文处看着像静默），JDBC 侧打出 [读取异常: Bad value for type int]。
-- libpq 侧已修复（聚合/表达式结果列按名恢复列元数据时硬编码 schema="public"，非 public
-- schema 的表查不到列元数据），现正常返回 -500/500；JDBC 侧为驱动自有实现，仍待修复。
-- 详见 plan/2026-09-20-HSM下OPE列min_max聚合两侧表现不一致.md
SELECT 'AGG: MIN/MAX OPE (HSM)' AS test_case;
SELECT min(ope_i4) AS min_val, max(ope_i4) AS max_val FROM swxa_full_test.enc_ope;

-- ============================================================
-- Part 5: UPDATE 操作
-- ============================================================

-- ── 5.1 按普通列条件UPDATE密态列 ──
SELECT '=== Part 5.1: UPDATE by plaintext condition（HSM） ===' AS stage;

UPDATE swxa_full_test.enc_all SET c_vc_det = 'Hello_HSM_Updated' WHERE tag = 'hsm_row1';
SELECT tag, c_vc_det FROM swxa_full_test.enc_all WHERE tag = 'hsm_row1';

-- ── 5.2 按DET密态列条件UPDATE ──
SELECT '=== Part 5.2: UPDATE by DET condition（HSM） ===' AS stage;

UPDATE swxa_full_test.enc_det SET det_vc = 'alpha_hsm_modified' WHERE det_i4 = 111;
SELECT tag, det_vc FROM swxa_full_test.enc_det WHERE tag = 'hsm_det_r1';

-- ── 5.3 按OPE范围条件UPDATE ──
SELECT '=== Part 5.3: UPDATE by OPE range（HSM） ===' AS stage;

UPDATE swxa_full_test.enc_ope SET tag = 'hsm_ope_updated' WHERE ope_i4 > 300;
SELECT ope_i4, tag FROM swxa_full_test.enc_ope WHERE ope_i4 > 300 ORDER BY ope_i4;

-- ── 5.4 按SEARCH LIKE条件UPDATE ──
SELECT '=== Part 5.4: UPDATE by SEARCH LIKE（HSM） ===' AS stage;

UPDATE swxa_full_test.enc_search SET tag = 'hsm_srch_updated' WHERE srch_txt LIKE '%engineer%';
SELECT srch_vc, tag FROM swxa_full_test.enc_search WHERE srch_txt LIKE '%engineer%';

-- ── 5.5 UPDATE多列 ──
SELECT '=== Part 5.5: UPDATE 多列（HSM） ===' AS stage;

UPDATE swxa_full_test.enc_all
SET c_int4 = 99999, c_f8_ope = 99.99, c_txt_all = 'HSM multi update test'
WHERE tag = 'hsm_row5';
SELECT tag, c_int4, c_f8_ope, c_txt_all FROM swxa_full_test.enc_all WHERE tag = 'hsm_row5';

-- ============================================================
-- Part 6: DELETE 操作
-- ============================================================

-- ── 6.1 按普通列条件DELETE ──
SELECT '=== Part 6.1: DELETE by plaintext（HSM） ===' AS stage;

INSERT INTO swxa_full_test.enc_all (c_int4, tag) VALUES (88888, 'hsm_to_delete_1');
DELETE FROM swxa_full_test.enc_all WHERE tag = 'hsm_to_delete_1';
SELECT 'DELETE plaintext: row deleted, count check (HSM)' AS test_case;
SELECT count(*) FROM swxa_full_test.enc_all WHERE tag = 'hsm_to_delete_1';

-- ── 6.2 按DET密态列等值DELETE ──
SELECT '=== Part 6.2: DELETE by DET（HSM） ===' AS stage;

INSERT INTO swxa_full_test.enc_det (det_i4, det_vc, tag) VALUES (9999, 'hsm_to_delete', 'hsm_del_by_det');
DELETE FROM swxa_full_test.enc_det WHERE det_vc = 'hsm_to_delete';
SELECT 'DELETE DET: row deleted, count check (HSM)' AS test_case;
SELECT count(*) FROM swxa_full_test.enc_det WHERE tag = 'hsm_del_by_det';

-- ── 6.3 按OPE范围DELETE ──
SELECT '=== Part 6.3: DELETE by OPE range（HSM） ===' AS stage;

INSERT INTO swxa_full_test.enc_ope (ope_i4, tag) VALUES (999, 'hsm_del_ope');
DELETE FROM swxa_full_test.enc_ope WHERE ope_i4 > 500;
SELECT 'DELETE OPE: rows > 500 deleted (HSM)' AS test_case;
SELECT count(*) FROM swxa_full_test.enc_ope WHERE ope_i4 > 500;

-- ── 6.4 按SEARCH LIKE DELETE ──
SELECT '=== Part 6.4: DELETE by SEARCH LIKE（HSM） ===' AS stage;

INSERT INTO swxa_full_test.enc_search (srch_txt, tag)
VALUES ('This HSM row will be deleted via LIKE', 'hsm_del_like');
DELETE FROM swxa_full_test.enc_search WHERE srch_txt LIKE '%deleted via LIKE%';
SELECT 'DELETE SEARCH: row deleted by LIKE (HSM)' AS test_case;
SELECT count(*) FROM swxa_full_test.enc_search WHERE tag = 'hsm_del_like';

-- ============================================================
-- Part 7: 最终验证
-- ============================================================

SELECT '=== Part 7: 最终全表验证（HSM） ===' AS stage;

SELECT 'All enc table final state (HSM)' AS test_case;
SELECT id, tag FROM swxa_full_test.enc_all ORDER BY id;

SELECT 'DET table final state (HSM)' AS test_case;
SELECT id, tag, det_vc FROM swxa_full_test.enc_det ORDER BY id;

SELECT 'OPE table final state (HSM)' AS test_case;
SELECT id, ope_i4, tag FROM swxa_full_test.enc_ope ORDER BY id;

SELECT 'SEARCH table final state (HSM)' AS test_case;
SELECT id, srch_vc, tag FROM swxa_full_test.enc_search ORDER BY id;

SELECT 'DEFAULT table final state (HSM)' AS test_case;
SELECT id, def_i4, def_vc, def_txt, note FROM swxa_full_test.enc_default ORDER BY id;

-- ============================================================
-- Part 8: 清理
-- ============================================================

SELECT '=== Part 8: 清理（HSM） ===' AS stage;

DROP SCHEMA IF EXISTS swxa_full_test CASCADE;
DROP DEK IF EXISTS swxa_full_dek CASCADE;
DROP KEK IF EXISTS swxa_full_kek CASCADE;
