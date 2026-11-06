-- TECC Encrypted Columns Regression Test
-- Test encrypted column creation and metadata recording
-- Created as part of TECC metadata implementation plan

-- Setup: Create test KEK and DEK
CREATE KEK encrypted_columns_test_kek WITH (
    KEY_STORE = 'localkms',
    KEY_PATH = '/tmp/encrypted_columns_test_key'
);

CREATE DEK encrypted_columns_test_dek WITH VALUES(
    KEK = 'encrypted_columns_test_kek',
    ENCRYPTED_VALUE = 'encrypted_columns_test_value',
    SUPPORT_ALGO = 'SM4'
);

-- Test 1: Create table with various encrypted column types
SELECT 'Test 1: Create table with encrypted columns' AS test_description;

CREATE TABLE encrypted_columns_test_table (
    id SERIAL PRIMARY KEY,
    plain_text TEXT,
    -- Different encrypted column types
    encrypted_int2 int2_enc,
    encrypted_int4 int4_enc,
    encrypted_int8 int8_enc,
    encrypted_float4 float4_enc,
    encrypted_float8 float8_enc,
    encrypted_numeric numeric_enc,
    encrypted_varchar varchar_enc,
    encrypted_bpchar bpchar_enc,
    encrypted_text text_enc,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

-- Verify table creation
SELECT 'Table created successfully' AS verification
WHERE EXISTS (SELECT 1 FROM pg_class WHERE relname = 'encrypted_columns_test_table');

-- Test 2: Verify encrypted column metadata (if recorded)
-- Note: This test may fail if metadata recording is not yet implemented
SELECT 'Test 2: Check encrypted column metadata' AS test_description;

SELECT
    (SELECT COUNT(*) FROM hg_encrypted_columns WHERE table_oid =
        (SELECT oid FROM pg_class WHERE relname = 'encrypted_columns_test_table')
    ) AS encrypted_columns_count;

-- Test 3: Basic operations on encrypted columns
SELECT 'Test 3: Basic operations on encrypted columns' AS test_description;

-- Note: Actual data operations may require TECC frontend processor
-- For now, we just verify the table structure
SELECT
    column_name,
    data_type,
    character_maximum_length,
    is_nullable
FROM information_schema.columns
WHERE table_name = 'encrypted_columns_test_table'
ORDER BY ordinal_position;

-- Test 4: Cleanup
SELECT 'Test 4: Cleanup' AS test_description;

DROP TABLE encrypted_columns_test_table;

-- Verify cleanup
SELECT 'Table dropped successfully' AS verification
WHERE NOT EXISTS (SELECT 1 FROM pg_class WHERE relname = 'encrypted_columns_test_table');

-- Drop test DEK and KEK
DROP DEK encrypted_columns_test_dek;
DROP KEK encrypted_columns_test_kek;

-- Final verification
SELECT 'TECC encrypted columns regression tests completed' AS test_status;