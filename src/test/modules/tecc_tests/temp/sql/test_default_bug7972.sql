-- Bug 7972: 加密列DEFAULT值大小写测试

CREATE KEK kek_default_test WITH (KEY_STORE = 'localkms', KEY_PATH = '/tmp/kek_default_test.key');
CREATE DEK dek_default_test WITH VALUES(KEK = kek_default_test);

-- 测试1: 小写 default
DROP TABLE IF EXISTS tb_text_default CASCADE;
CREATE TABLE tb_text_default(
    id SERIAL PRIMARY KEY,
    short_text text_enc encrypted with (dek = dek_default_test),
    default_text text_enc encrypted with (dek = dek_default_test) default 'defaultvalue'
);

-- 测试2: 大写 DEFAULT
DROP TABLE IF EXISTS tb_upper_default CASCADE;
CREATE TABLE tb_upper_default(
    id SERIAL PRIMARY KEY,
    val text_enc encrypted with (dek = dek_default_test) DEFAULT 'upper_value'
);

-- 测试3: 混合大小写 DeFaUlT
DROP TABLE IF EXISTS tb_mixed_default CASCADE;
CREATE TABLE tb_mixed_default(
    id SERIAL PRIMARY KEY,
    val text_enc encrypted with (dek = dek_default_test) DeFaUlT 'mixed_case'
);

-- 清理
DROP TABLE IF EXISTS tb_text_default CASCADE;
DROP TABLE IF EXISTS tb_upper_default CASCADE;
DROP TABLE IF EXISTS tb_mixed_default CASCADE;
DROP DEK dek_default_test;
DROP KEK kek_default_test;
