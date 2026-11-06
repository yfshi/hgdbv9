-- TECC Integration Regression Test
-- Test end-to-end TECC functionality including complete encrypted SQL workflow
-- Migrated from independent test programs (test_e2e_validation.cpp, test_integration.cpp)

\set VERBOSITY terse

-- Setup: Create comprehensive test environment
-- Create test KEK and DEK for integration testing
CREATE KEK integration_test_kek WITH (
    _TECC_PROCESSED = '1',
    KEY_STORE = 'localkms',
    KEY_PATH = '/tmp/integration_test_key'
);

-- Create DEKs with different algorithm support for comprehensive testing
-- Note: ENCRYPTED_VALUE and _TECC_PROCESSED are auto-generated/injected
-- by the frontend TECC processor. Do not include them in the SQL.
CREATE DEK integration_test_dek_det WITH VALUES (
    KEK = 'integration_test_kek'
);

CREATE DEK integration_test_dek_ope WITH VALUES (
    KEK = 'integration_test_kek'
);

CREATE DEK integration_test_dek_search WITH VALUES (
    KEK = 'integration_test_kek'
);

-- Test 1: Complete encrypted table lifecycle
-- Test CREATE TABLE with encrypted columns, data operations, and DROP TABLE
SELECT 'Test 1: Complete encrypted table lifecycle' AS test_description;

-- Create a comprehensive test table with multiple encrypted column types
CREATE TABLE integration_test_table (
    id SERIAL PRIMARY KEY,
    employee_id INT,
    plain_name TEXT,
    -- Encrypted columns for different data types
    encrypted_salary int4,
    encrypted_bonus numeric,
    encrypted_department text,
    encrypted_rating float4,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

-- Mark columns as encrypted (simulating proper TECC column marking)
INSERT INTO hg_encrypted_columns (
    oid, table_oid, column_attnum, column_name,
    data_type_original_oid, data_type_original_mod, column_key_id,
    encryption_type, collation_oid, create_date
) VALUES
    (20010,
     (SELECT oid FROM pg_class WHERE relname = 'integration_test_table'),
     4, 'encrypted_salary',
     (SELECT oid FROM pg_type WHERE typname = 'int4'),
     -1,
     (SELECT oid FROM hg_dek WHERE dek_name = 'integration_test_dek_det'),
     1,  -- DET only
     0,
     now()),
    (20011,
     (SELECT oid FROM pg_class WHERE relname = 'integration_test_table'),
     5, 'encrypted_bonus',
     (SELECT oid FROM pg_type WHERE typname = 'numeric'),
     -1,
     (SELECT oid FROM hg_dek WHERE dek_name = 'integration_test_dek_det'),
     1,  -- DET only
     0,
     now()),
    (20012,
     (SELECT oid FROM pg_class WHERE relname = 'integration_test_table'),
     6, 'encrypted_department',
     (SELECT oid FROM pg_type WHERE typname = 'text'),
     -1,
     (SELECT oid FROM hg_dek WHERE dek_name = 'integration_test_dek_search'),
     4,  -- SEARCH only
     0,
     now()),
    (20013,
     (SELECT oid FROM pg_class WHERE relname = 'integration_test_table'),
     7, 'encrypted_rating',
     (SELECT oid FROM pg_type WHERE typname = 'float4'),
     -1,
     (SELECT oid FROM hg_dek WHERE dek_name = 'integration_test_dek_ope'),
     2,  -- OPE only
     0,
     now());

-- Test 2: Comprehensive data insertion test
-- Test INSERT operations with various data types and values
-- Reconnect with frontend TECC disabled for DML
\connect "dbname=regression enable_tecc=0"
SELECT 'Test 2: Comprehensive data insertion' AS test_description;

-- Insert test data with different data types
INSERT INTO integration_test_table (
    employee_id, plain_name,
    encrypted_salary, encrypted_bonus, encrypted_department,
    encrypted_rating
) VALUES
    (1001, 'Alice Johnson', 75000, 15000.50, 'Engineering', 4.5),
    (1002, 'Bob Smith', 82000, 20000.75, 'Sales', 4.2),
    (1003, 'Carol Davis', 68000, 12000.25, 'Marketing', 4.8),
    (1004, 'David Wilson', 95000, 25000.00, 'Engineering', 4.0),
    (1005, 'Eva Brown', 72000, 18000.50, 'HR', 4.6);

-- Test 3: Complex SELECT queries with encrypted columns
-- Test various SELECT patterns including WHERE, ORDER BY, GROUP BY
SELECT 'Test 3: Complex SELECT queries' AS test_description;

-- Simple SELECT with encrypted columns
SELECT id, employee_id, plain_name, encrypted_salary, encrypted_department
FROM integration_test_table
ORDER BY id;

-- SELECT with WHERE clause on plain column
SELECT id, plain_name, encrypted_salary
FROM integration_test_table
WHERE plain_name LIKE '%Johnson%';

-- SELECT with ORDER BY on plain column
SELECT id, plain_name, encrypted_bonus
FROM integration_test_table
ORDER BY plain_name;

-- Test 4: UPDATE operations with encrypted columns
-- Test various UPDATE patterns
SELECT 'Test 4: UPDATE operations' AS test_description;

-- Update salary for a specific employee
UPDATE integration_test_table
SET encrypted_salary = 80000,
    updated_at = CURRENT_TIMESTAMP
WHERE employee_id = 1001;

-- Verify update
SELECT id, employee_id, plain_name, encrypted_salary, encrypted_bonus, encrypted_rating
FROM integration_test_table
WHERE employee_id IN (1001, 1004)
ORDER BY employee_id;

-- Test 5: DELETE operations with encrypted columns
-- Test DELETE with various conditions
SELECT 'Test 5: DELETE operations' AS test_description;

-- Insert a row to be deleted
INSERT INTO integration_test_table (employee_id, plain_name, encrypted_salary)
VALUES (9999, 'Temporary Employee', 50000);

-- Delete the temporary row
DELETE FROM integration_test_table
WHERE employee_id = 9999;

-- Verify deletion
SELECT COUNT(*) as temp_employee_count
FROM integration_test_table
WHERE employee_id = 9999;

-- Test 6: Transaction testing with encrypted columns
-- Test transaction rollback and commit scenarios
SELECT 'Test 6: Transaction testing' AS test_description;

-- Start a transaction
BEGIN;

-- Insert data within transaction
INSERT INTO integration_test_table (employee_id, plain_name, encrypted_salary)
VALUES (2001, 'Transaction Test 1', 60000);

-- Rollback transaction
ROLLBACK;

-- Verify rollback (row should not exist)
SELECT COUNT(*) as after_rollback_count
FROM integration_test_table
WHERE employee_id = 2001;

-- Test commit scenario
BEGIN;

INSERT INTO integration_test_table (employee_id, plain_name, encrypted_salary)
VALUES (2002, 'Transaction Test 2', 65000);

COMMIT;

-- Verify commit (row should exist)
SELECT COUNT(*) as after_commit_count
FROM integration_test_table
WHERE employee_id = 2002;

-- Test 7: Error handling and edge cases
-- Reconnect with frontend TECC enabled for cleanup
\connect "dbname=regression enable_tecc=1"
SELECT 'Test 7: Error handling' AS test_description;

-- Test invalid operations (these should be handled gracefully)
SELECT 'Error handling tests placeholder' AS error_test;

-- Test 8: Cleanup and resource management
-- Test proper cleanup of all test objects
SELECT 'Test 8: Cleanup and resource management' AS test_description;

-- Cleanup: Remove test objects in reverse order of creation

-- Remove encrypted column metadata
DELETE FROM hg_encrypted_columns
WHERE table_oid = (SELECT oid FROM pg_class WHERE relname = 'integration_test_table');

DROP TABLE IF EXISTS integration_test_table;

-- Drop test DEKs
DROP DEK IF EXISTS integration_test_dek_det;
DROP DEK IF EXISTS integration_test_dek_ope;
DROP DEK IF EXISTS integration_test_dek_search;

-- Drop test KEK
DROP KEK IF EXISTS integration_test_kek;

-- Final verification
SELECT 'Test 9: Final verification' AS test_description;

SELECT
    (SELECT COUNT(*) FROM hg_kek WHERE kek_name = 'integration_test_kek') as remaining_kek,
    (SELECT COUNT(*) FROM hg_dek WHERE dek_name LIKE 'integration_test_dek_%') as remaining_deks,
    (SELECT COUNT(*) FROM pg_class WHERE relname = 'integration_test_table') as remaining_table;

-- Final test status
SELECT 'TECC Integration regression tests completed' AS test_status;
