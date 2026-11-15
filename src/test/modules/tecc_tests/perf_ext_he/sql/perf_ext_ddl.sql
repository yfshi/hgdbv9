-- perf_ext_ddl.sql
-- Customer-like employee model for TECC extended performance tests.

DROP TABLE IF EXISTS employee_plain CASCADE;
DROP TABLE IF EXISTS employee_full_enc CASCADE;
DROP TABLE IF EXISTS employee_half_enc CASCADE;
DROP TABLE IF EXISTS stage_employee_plain CASCADE;
DROP TABLE IF EXISTS stage_employee_full_enc CASCADE;
DROP TABLE IF EXISTS stage_employee_half_enc CASCADE;
DROP DEK IF EXISTS dek_perf_ext_he_add CASCADE;
DROP DEK IF EXISTS dek_perf_ext_he_mul CASCADE;
DROP DEK IF EXISTS dek_perf_ext CASCADE;
DROP KEK IF EXISTS kek_perf_ext_he CASCADE;
DROP KEK IF EXISTS kek_perf_ext CASCADE;

CREATE KEK kek_perf_ext WITH (
    key_store = 'localkms',
    key_path = '%KEKPATH%/kek_perf_ext.key'
);

CREATE DEK dek_perf_ext WITH VALUES (
    KEK = kek_perf_ext
);

CREATE KEK kek_perf_ext_he WITH (
    key_store = 'localkms',
    key_algo = 'sm2',
    key_path = '%KEKPATH%/kek_perf_ext_he.priv'
);

CREATE DEK dek_perf_ext_he_add WITH VALUES (
    KEK = kek_perf_ext_he,
    KEY_ALGO = 'he-add'
);

CREATE DEK dek_perf_ext_he_mul WITH VALUES (
    KEK = kek_perf_ext_he,
    KEY_ALGO = 'he-mul'
);

CREATE TABLE employee_plain (
    row_id BIGINT,
    employee_id INTEGER,
    name VARCHAR(80),
    salary NUMERIC(12,2),
    address TEXT,
    phone VARCHAR(32),
    idcard_no VARCHAR(32),
    department VARCHAR(64),
    job_title VARCHAR(64),
    hire_year INTEGER,
    age INTEGER,
    grade SMALLINT,
    bonus NUMERIC(12,2),
    city VARCHAR(64),
    email TEXT,
    status SMALLINT,
    manager_id INTEGER,
    performance_score DOUBLE PRECISION,
    remark TEXT
);

CREATE TABLE employee_full_enc (
    row_id BIGINT,
    employee_id INT4_ENC ENCRYPTED WITH (DEK = 'dek_perf_ext'),
    name VARCHAR_ENC ENCRYPTED WITH (DEK = 'dek_perf_ext'),
    salary NUMERIC_ENC ENCRYPTED WITH (DEK = 'dek_perf_ext'),
    address TEXT_ENC ENCRYPTED WITH (DEK = 'dek_perf_ext'),
    phone VARCHAR_ENC ENCRYPTED WITH (DEK = 'dek_perf_ext'),
    idcard_no VARCHAR_ENC ENCRYPTED WITH (DEK = 'dek_perf_ext'),
    department VARCHAR_ENC ENCRYPTED WITH (DEK = 'dek_perf_ext'),
    job_title VARCHAR_ENC ENCRYPTED WITH (DEK = 'dek_perf_ext'),
    hire_year INT4_ENC ENCRYPTED WITH (DEK = 'dek_perf_ext'),
    age INT8_ENC ENCRYPTED WITH (HE_ADD_DEK = 'dek_perf_ext_he_add', SUPPORT_ALGO = 'he-add'),
    grade INT8_ENC ENCRYPTED WITH (HE_MUL_DEK = 'dek_perf_ext_he_mul', SUPPORT_ALGO = 'he-mul'),
    bonus NUMERIC_ENC ENCRYPTED WITH (DEK = 'dek_perf_ext'),
    city VARCHAR_ENC ENCRYPTED WITH (DEK = 'dek_perf_ext'),
    email TEXT_ENC ENCRYPTED WITH (DEK = 'dek_perf_ext'),
    status INT2_ENC ENCRYPTED WITH (DEK = 'dek_perf_ext'),
    manager_id INT4_ENC ENCRYPTED WITH (DEK = 'dek_perf_ext'),
    performance_score FLOAT8_ENC ENCRYPTED WITH (DEK = 'dek_perf_ext'),
    remark TEXT_ENC ENCRYPTED WITH (DEK = 'dek_perf_ext')
);

CREATE TABLE employee_half_enc (
    row_id BIGINT,
    employee_id INTEGER,
    name VARCHAR_ENC ENCRYPTED WITH (DEK = 'dek_perf_ext'),
    salary NUMERIC_ENC ENCRYPTED WITH (DEK = 'dek_perf_ext'),
    address TEXT_ENC ENCRYPTED WITH (DEK = 'dek_perf_ext'),
    phone VARCHAR_ENC ENCRYPTED WITH (DEK = 'dek_perf_ext'),
    idcard_no VARCHAR_ENC ENCRYPTED WITH (DEK = 'dek_perf_ext'),
    department VARCHAR(64),
    job_title VARCHAR(64),
    hire_year INTEGER,
    age INT8_ENC ENCRYPTED WITH (HE_ADD_DEK = 'dek_perf_ext_he_add', SUPPORT_ALGO = 'he-add'),
    grade SMALLINT,
    bonus NUMERIC_ENC ENCRYPTED WITH (DEK = 'dek_perf_ext'),
    city VARCHAR(64),
    email TEXT_ENC ENCRYPTED WITH (DEK = 'dek_perf_ext'),
    status SMALLINT,
    manager_id INTEGER,
    performance_score FLOAT8_ENC ENCRYPTED WITH (DEK = 'dek_perf_ext'),
    remark TEXT
);

CREATE TABLE stage_employee_plain (LIKE employee_plain);

CREATE TABLE stage_employee_full_enc (
    row_id BIGINT,
    employee_id INT4_ENC ENCRYPTED WITH (DEK = 'dek_perf_ext'),
    name VARCHAR_ENC ENCRYPTED WITH (DEK = 'dek_perf_ext'),
    salary NUMERIC_ENC ENCRYPTED WITH (DEK = 'dek_perf_ext'),
    address TEXT_ENC ENCRYPTED WITH (DEK = 'dek_perf_ext'),
    phone VARCHAR_ENC ENCRYPTED WITH (DEK = 'dek_perf_ext'),
    idcard_no VARCHAR_ENC ENCRYPTED WITH (DEK = 'dek_perf_ext'),
    department VARCHAR_ENC ENCRYPTED WITH (DEK = 'dek_perf_ext'),
    job_title VARCHAR_ENC ENCRYPTED WITH (DEK = 'dek_perf_ext'),
    hire_year INT4_ENC ENCRYPTED WITH (DEK = 'dek_perf_ext'),
    age INT8_ENC ENCRYPTED WITH (HE_ADD_DEK = 'dek_perf_ext_he_add', SUPPORT_ALGO = 'he-add'),
    grade INT8_ENC ENCRYPTED WITH (HE_MUL_DEK = 'dek_perf_ext_he_mul', SUPPORT_ALGO = 'he-mul'),
    bonus NUMERIC_ENC ENCRYPTED WITH (DEK = 'dek_perf_ext'),
    city VARCHAR_ENC ENCRYPTED WITH (DEK = 'dek_perf_ext'),
    email TEXT_ENC ENCRYPTED WITH (DEK = 'dek_perf_ext'),
    status INT2_ENC ENCRYPTED WITH (DEK = 'dek_perf_ext'),
    manager_id INT4_ENC ENCRYPTED WITH (DEK = 'dek_perf_ext'),
    performance_score FLOAT8_ENC ENCRYPTED WITH (DEK = 'dek_perf_ext'),
    remark TEXT_ENC ENCRYPTED WITH (DEK = 'dek_perf_ext')
);

CREATE TABLE stage_employee_half_enc (
    row_id BIGINT,
    employee_id INTEGER,
    name VARCHAR_ENC ENCRYPTED WITH (DEK = 'dek_perf_ext'),
    salary NUMERIC_ENC ENCRYPTED WITH (DEK = 'dek_perf_ext'),
    address TEXT_ENC ENCRYPTED WITH (DEK = 'dek_perf_ext'),
    phone VARCHAR_ENC ENCRYPTED WITH (DEK = 'dek_perf_ext'),
    idcard_no VARCHAR_ENC ENCRYPTED WITH (DEK = 'dek_perf_ext'),
    department VARCHAR(64),
    job_title VARCHAR(64),
    hire_year INTEGER,
    age INT8_ENC ENCRYPTED WITH (HE_ADD_DEK = 'dek_perf_ext_he_add', SUPPORT_ALGO = 'he-add'),
    grade SMALLINT,
    bonus NUMERIC_ENC ENCRYPTED WITH (DEK = 'dek_perf_ext'),
    city VARCHAR(64),
    email TEXT_ENC ENCRYPTED WITH (DEK = 'dek_perf_ext'),
    status SMALLINT,
    manager_id INTEGER,
    performance_score FLOAT8_ENC ENCRYPTED WITH (DEK = 'dek_perf_ext'),
    remark TEXT
);

CREATE INDEX idx_employee_plain_row_id ON employee_plain (row_id);
CREATE INDEX idx_employee_full_row_id ON employee_full_enc (row_id);
CREATE INDEX idx_employee_half_row_id ON employee_half_enc (row_id);

SELECT 'perf_ext_setup_done';
