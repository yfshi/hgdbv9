-- TECC Key Management Regression Test
-- Test key lifecycle, dependencies, and option combinations

-- Test 1: Create KEK with various options
CREATE KEK km_test_kek_1 WITH (
    _TECC_PROCESSED = '1',
    KEY_STORE = 'localkms',
    KEY_PATH = '/tmp/km_key_1',
    ALGORITHM = 'SM4',
    KEY_LENGTH = '256'
);

-- Verify KEK created with all options
SELECT kek_name, key_namespace FROM hg_kek WHERE kek_name = 'km_test_kek_1';

-- Verify all KEK args stored
SELECT key, value FROM hg_kek_args ka
JOIN hg_kek k ON ka.kek_id = k.oid
WHERE k.kek_name = 'km_test_kek_1'
	AND key != '_KEK_META'
ORDER BY key;

-- Test 2: Create another KEK with minimal options
CREATE KEK km_test_kek_2 WITH (
    _TECC_PROCESSED = '1',
    KEY_STORE = 'localkms',
    KEY_PATH = '/tmp/km_key_2'
);

-- Test 3: Create multiple DEKs referencing the same KEK
CREATE DEK km_test_dek_1 WITH VALUES (
    _TECC_PROCESSED = '1',
    ENCRYPTED_VALUE = 'deadbeef12345678abcdef10',
    KEK = 'km_test_kek_1'
);

CREATE DEK km_test_dek_2 WITH VALUES (
    _TECC_PROCESSED = '1',
    ENCRYPTED_VALUE = 'deadbeef12345678abcdef11',
    KEK = 'km_test_kek_1',
    KEY_LENGTH = '128'
);

CREATE DEK km_test_dek_3 WITH VALUES (
    _TECC_PROCESSED = '1',
    ENCRYPTED_VALUE = 'deadbeef12345678abcdef12',
    KEK = 'km_test_kek_2'
);

-- Test 4: Verify DEKs created with correct KEK references
SELECT d.dek_name, k.kek_name as kek_name
FROM hg_dek d
JOIN hg_kek k ON d.kek_id = k.oid
WHERE d.dek_name LIKE 'km_test_dek_%'
ORDER BY d.dek_name;

-- Test 5: Verify DEK args stored correctly
SELECT d.dek_name, da.key, encode(da.value, 'escape') as value
FROM hg_dek_args da
JOIN hg_dek d ON da.dek_id = d.oid
WHERE d.dek_name LIKE 'km_test_dek_%'
ORDER BY d.dek_name, da.key;

-- Test 6: Create DEK with SUPPORT_ALGO option
CREATE DEK km_test_dek_algo WITH VALUES (
    _TECC_PROCESSED = '1',
    ENCRYPTED_VALUE = 'deadbeef12345678abcdef13',
    KEK = 'km_test_kek_2',
    SUPPORT_ALGO = 'SM4'
);

-- Verify SUPPORT_ALGO stored correctly
SELECT d.dek_name, da.key, encode(da.value, 'escape') as value
FROM hg_dek_args da
JOIN hg_dek d ON da.dek_id = d.oid
WHERE d.dek_name = 'km_test_dek_algo'
ORDER BY da.key;

-- Test 7: Dependency enforcement — dropping KEK with DEKs should fail
DROP KEK km_test_kek_1;

-- Test 8: Drop DEKs then KEK
DROP DEK km_test_dek_1;
DROP DEK km_test_dek_2;

-- Now KEK can be dropped
DROP KEK km_test_kek_1;

-- Verify cleanup
SELECT COUNT(*) FROM hg_kek WHERE kek_name = 'km_test_kek_1';
SELECT COUNT(*) FROM hg_dek WHERE dek_name LIKE 'km_test_dek_%' AND dek_name != 'km_test_dek_3';

-- Test 9: Create KEK with schema-qualified reference
CREATE KEK km_test_kek_schema WITH (
    _TECC_PROCESSED = '1',
    KEY_STORE = 'localkms',
    KEY_PATH = '/tmp/km_schema_key'
);

-- Reference KEK by schema-qualified name in DEK creation
CREATE DEK km_test_dek_schema WITH VALUES (
    _TECC_PROCESSED = '1',
    ENCRYPTED_VALUE = 'deadbeef12345678abcdef14',
    KEK = 'km_test_kek_schema'
);

-- Verify via hg_kek_info view
SELECT kek_name, key_namespace_name
FROM hg_kek_info
WHERE kek_name = 'km_test_kek_schema';

-- Cleanup schema test
DROP DEK km_test_dek_schema;
DROP KEK km_test_kek_schema;

-- Test 10: Duplicate name error handling
CREATE KEK km_test_dup WITH (
    _TECC_PROCESSED = '1',
    KEY_STORE = 'localkms',
    KEY_PATH = '/tmp/km_dup_key'
);
-- Try to create same KEK again (should fail with duplicate error)
CREATE KEK km_test_dup WITH (
    _TECC_PROCESSED = '1',
    KEY_STORE = 'localkms',
    KEY_PATH = '/tmp/km_dup_key'
);
-- IF NOT EXISTS should skip with notice
CREATE KEK IF NOT EXISTS km_test_dup WITH (
    _TECC_PROCESSED = '1',
    KEY_STORE = 'localkms',
    KEY_PATH = '/tmp/km_dup_key'
);
DROP KEK km_test_dup;

-- Test 10b: Duplicate filename in different directory (should fail)
CREATE KEK km_test_dup_filename WITH (
    _TECC_PROCESSED = '1',
    KEY_STORE = 'localkms',
    KEY_PATH = '/tmp/km_dup_key'
);
-- Try to create a different KEK with a different path but same filename
CREATE KEK km_test_other_path WITH (
    _TECC_PROCESSED = '1',
    KEY_STORE = 'localkms',
    KEY_PATH = '/var/tmp/km_dup_key'
);
DROP KEK km_test_dup_filename;

-- Test 11: Create DEK with non-existent KEK (should fail)
CREATE DEK km_test_bad_dek WITH VALUES (
    _TECC_PROCESSED = '1',
    ENCRYPTED_VALUE = 'deadbeef12345678abcdef30',
    KEK = 'nonexistent_kek_for_test'
);

-- Test 12: DROP KEK ... CASCADE with dependent DEKs
CREATE KEK km_test_cascade_kek WITH (
    _TECC_PROCESSED = '1',
    KEY_STORE = 'localkms',
    KEY_PATH = '/tmp/km_cascade_key'
);
CREATE DEK km_test_cascade_dek WITH VALUES (
    _TECC_PROCESSED = '1',
    ENCRYPTED_VALUE = 'deadbeef12345678abcdef20',
    KEK = 'km_test_cascade_kek'
);
-- CASCADE should drop both KEK and dependent DEK
DROP KEK km_test_cascade_kek CASCADE;
-- Verify both dropped
SELECT COUNT(*) FROM hg_kek WHERE kek_name = 'km_test_cascade_kek';
SELECT COUNT(*) FROM hg_dek WHERE dek_name = 'km_test_cascade_dek';

-- Test 13: Cleanup remaining objects
DROP DEK km_test_dek_3;
DROP DEK km_test_dek_algo;
DROP KEK km_test_kek_2;

-- Final verification
SELECT COUNT(*) as remaining_kek FROM hg_kek WHERE kek_name LIKE 'km_test_kek_%';
SELECT COUNT(*) as remaining_dek FROM hg_dek WHERE dek_name LIKE 'km_test_dek_%';
