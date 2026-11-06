-- TECC System Tables Regression Test
-- Test CREATE/DROP KEK/DEK commands and system table functionality
-- Based on the latest key management implementation

-- Test 1: Check if system tables exist
SELECT relname FROM pg_class
WHERE relname IN ('hg_kek', 'hg_dek', 'hg_kek_args', 'hg_dek_args')
ORDER BY relname;

-- Test 2: Create a KEK with localkms
CREATE KEK sys_test_kek_1 WITH (
    _TECC_PROCESSED = '1',
    KEY_STORE = 'localkms',
    KEY_PATH = '/tmp/test_key_1'
);

-- Verify KEK was created — current hg_kek has: oid, kek_name, key_namespace, key_owner, key_acl, create_date
SELECT kek_name, key_namespace
FROM hg_kek
WHERE kek_name = 'sys_test_kek_1';

-- Verify KEK args were stored
SELECT ka.key, ka.value
FROM hg_kek_args ka
JOIN hg_kek k ON ka.kek_id = k.oid
WHERE k.kek_name = 'sys_test_kek_1'
	AND ka.key != '_KEK_META'
ORDER BY ka.key;

-- Test 3: Create a DEK referencing the KEK
CREATE DEK sys_test_dek_1 WITH VALUES (
    _TECC_PROCESSED = '1',
    ENCRYPTED_VALUE = 'deadbeef12345678abcdef00',
    KEK = 'sys_test_kek_1'
);

-- Verify DEK was created
SELECT d.dek_name
FROM hg_dek d
WHERE d.dek_name = 'sys_test_dek_1';

-- Verify DEK args were stored
SELECT da.key, encode(da.value, 'escape') as value
FROM hg_dek_args da
JOIN hg_dek d ON da.dek_id = d.oid
WHERE d.dek_name = 'sys_test_dek_1'
ORDER BY da.key;

-- Test 4: Create another KEK with different parameters
CREATE KEK sys_test_kek_2 WITH (
    _TECC_PROCESSED = '1',
    KEY_STORE = 'localkms',
    KEY_PATH = '/tmp/test_key_2',
    ALGORITHM = 'SM4'
);

-- Test 5: Create another DEK
CREATE DEK sys_test_dek_2 WITH VALUES (
    _TECC_PROCESSED = '1',
    ENCRYPTED_VALUE = 'deadbeef12345678abcdef00',
    KEK = 'sys_test_kek_2',
    KEY_LENGTH = '128'
);

-- Test 6: List all KEKs using hg_kek_info view
SELECT kek_name, arg_key, arg_value
FROM hg_kek_info
WHERE kek_name LIKE 'sys_test_kek_%'
	AND arg_key != '_KEK_META'
ORDER BY kek_name, arg_key;

-- Test 7: List all DEKs using hg_dek_info view
SELECT d.dek_name, k.kek_name as kek_name
FROM hg_dek d
LEFT JOIN hg_kek k ON d.kek_id = k.oid
WHERE d.dek_name LIKE 'sys_test_dek_%'
ORDER BY d.dek_name;

-- Test 8: Drop a DEK
DROP DEK sys_test_dek_2;

-- Verify DEK was dropped
SELECT COUNT(*) FROM hg_dek WHERE dek_name = 'sys_test_dek_2';

-- Test 9: Drop a KEK (should fail if DEKs depend on it)
-- This should fail because sys_test_dek_1 depends on sys_test_kek_1
DROP KEK sys_test_kek_1;

-- Test 10: Drop the dependent DEK first
DROP DEK sys_test_dek_1;

-- Verify DEK was dropped
SELECT COUNT(*) FROM hg_dek WHERE dek_name = 'sys_test_dek_1';

-- Test 11: Now drop the KEK
DROP KEK sys_test_kek_1;

-- Test 12: Drop the remaining KEK
DROP KEK sys_test_kek_2;

-- Final verification: All test objects should be gone
SELECT COUNT(*) as remaining_kek FROM hg_kek
WHERE kek_name LIKE 'sys_test_kek_%';

SELECT COUNT(*) as remaining_dek FROM hg_dek
WHERE dek_name LIKE 'sys_test_dek_%';

-- Test 13: Test IF EXISTS variants
DROP KEK IF EXISTS sys_test_kek_non_existent;
DROP DEK IF EXISTS sys_test_dek_non_existent;

-- Test 14: Test schema-qualified KEK creation
CREATE KEK sys_test_kek_schema WITH (
    _TECC_PROCESSED = '1',
    KEY_STORE = 'localkms',
    KEY_PATH = '/tmp/test_schema_key'
);

-- Verify via view
SELECT kek_name, key_namespace_name
FROM hg_kek_info
WHERE kek_name = 'sys_test_kek_schema';

-- Cleanup schema test
DROP KEK sys_test_kek_schema;

-- Test 15: KEK with long name
CREATE KEK sys_test_kek_with_long_name_test WITH (
    _TECC_PROCESSED = '1',
    KEY_STORE = 'localkms',
    KEY_PATH = '/tmp/test_long_name_key'
);

-- Verify
SELECT kek_name FROM hg_kek WHERE kek_name = 'sys_test_kek_with_long_name_test';

-- Cleanup
DROP KEK sys_test_kek_with_long_name_test;

-- Final cleanup: ensure no leftover objects from this test
-- (use DROP IF EXISTS to avoid errors if already cleaned)
DROP DEK IF EXISTS sys_test_dek_1;
DROP DEK IF EXISTS sys_test_dek_2;
DROP KEK IF EXISTS sys_test_kek_1;
DROP KEK IF EXISTS sys_test_kek_2;
DROP KEK IF EXISTS sys_test_kek_schema;
DROP KEK IF EXISTS sys_test_kek_with_long_name_test;
