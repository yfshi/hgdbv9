-- ke跨驱动测试：KEK/DEK + 加密表创建
DROP KEK IF EXISTS kek_cross CASCADE;

CREATE KEK kek_cross WITH (
    KEY_STORE = 'localkms',
    KEY_PATH = '/tmp/kek_cross.key'
);

CREATE DEK dek_det  WITH VALUES(KEK = kek_cross);
CREATE DEK dek_rnd  WITH VALUES(KEK = kek_cross);
CREATE DEK dek_ope  WITH VALUES(KEK = kek_cross);
CREATE DEK dek_search WITH VALUES(KEK = kek_cross);

-- RND表
DROP TABLE IF EXISTS cross_rnd CASCADE;
CREATE TABLE cross_rnd (
    id SERIAL,
    c_int2  int2_enc    ENCRYPTED WITH (DEK = dek_rnd),
    c_int4  int4_enc    ENCRYPTED WITH (DEK = dek_rnd),
    c_int8  int8_enc    ENCRYPTED WITH (DEK = dek_rnd),
    c_float4  float4_enc   ENCRYPTED WITH (DEK = dek_rnd),
    c_float8  float8_enc   ENCRYPTED WITH (DEK = dek_rnd),
    c_numeric numeric_enc  ENCRYPTED WITH (DEK = dek_rnd),
    c_text  text_enc     ENCRYPTED WITH (DEK = dek_rnd),
    c_varchar varchar_enc ENCRYPTED WITH (DEK = dek_rnd),
    c_bpchar bpchar_enc   ENCRYPTED WITH (DEK = dek_rnd)
);

-- DET表
DROP TABLE IF EXISTS cross_det CASCADE;
CREATE TABLE cross_det (
    id SERIAL,
    c_int4     int4_enc     ENCRYPTED WITH (DEK = dek_det, SUPPORT_ALGO = 'det'),
    c_float8   float8_enc   ENCRYPTED WITH (DEK = dek_det, SUPPORT_ALGO = 'det'),
    c_numeric  numeric_enc  ENCRYPTED WITH (DEK = dek_det, SUPPORT_ALGO = 'det'),
    c_text     text_enc     ENCRYPTED WITH (DEK = dek_det, SUPPORT_ALGO = 'det'),
    c_varchar  varchar_enc  ENCRYPTED WITH (DEK = dek_det, SUPPORT_ALGO = 'det')
);

-- OPE表
DROP TABLE IF EXISTS cross_ope CASCADE;
CREATE TABLE cross_ope (
    id SERIAL,
    c_int4  int4_enc    ENCRYPTED WITH (DEK = dek_ope, SUPPORT_ALGO = 'ope'),
    c_numeric numeric_enc ENCRYPTED WITH (DEK = dek_ope, SUPPORT_ALGO = 'ope'),
    c_text  text_enc     ENCRYPTED WITH (DEK = dek_ope, SUPPORT_ALGO = 'ope')
);

-- SEARCH表
DROP TABLE IF EXISTS cross_search CASCADE;
CREATE TABLE cross_search (
    id SERIAL,
    c_text  text_enc     ENCRYPTED WITH (DEK = dek_search, SUPPORT_ALGO = 'search'),
    c_varchar varchar_enc ENCRYPTED WITH (DEK = dek_search, SUPPORT_ALGO = 'search')
);
