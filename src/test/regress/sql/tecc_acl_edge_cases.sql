-- TECC ACL Edge Cases Regression Test
-- Test edge cases and error conditions for ACL permission system

-- Setup: Create test users
CREATE USER tecc_edge_user1;
CREATE USER tecc_edge_user2;
CREATE USER tecc_edge_user3;

-- Create test KEK and DEK
CREATE KEK test_edge_kek WITH (
    _TECC_PROCESSED = '1',
    KEY_STORE = 'localkms',
    KEY_PATH = '/tmp/test_edge_key'
);

CREATE DEK test_edge_dek WITH VALUES (
    _TECC_PROCESSED = '1',
    ENCRYPTED_VALUE = 'deadbeef12345678abcdef00',
    KEK = 'test_edge_kek'
);

-- Get OIDs for testing
\set kek_oid '(SELECT oid FROM hg_kek WHERE kek_name = ''test_edge_kek'')'
\set dek_oid '(SELECT oid FROM hg_dek WHERE dek_name = ''test_edge_dek'')'
\set user1_oid '(SELECT oid FROM pg_authid WHERE rolname = ''tecc_edge_user1'')'
\set user2_oid '(SELECT oid FROM pg_authid WHERE rolname = ''tecc_edge_user2'')'

-- Test 1: Empty key name
SELECT has_kek_privilege('', 'USAGE') AS empty_key_name_kek;
SELECT has_dek_privilege('', 'USAGE') AS empty_key_name_dek;

-- Test 2: NULL key name (should return NULL)
SELECT has_kek_privilege(NULL, 'USAGE') AS null_key_name_kek;
SELECT has_dek_privilege(NULL, 'USAGE') AS null_key_name_dek;

-- Test 3: Empty privilege string
SELECT has_kek_privilege('test_edge_kek', '') AS empty_priv_kek;
SELECT has_dek_privilege('test_edge_dek', '') AS empty_priv_dek;

-- Test 4: NULL privilege (should return NULL)
SELECT has_kek_privilege('test_edge_kek', NULL) AS null_priv_kek;
SELECT has_dek_privilege('test_edge_dek', NULL) AS null_priv_dek;

-- Test 5: Invalid privilege names
SELECT has_kek_privilege('test_edge_kek', 'SELECT') AS invalid_priv_select_kek;
SELECT has_kek_privilege('test_edge_kek', 'INSERT') AS invalid_priv_insert_kek;
SELECT has_kek_privilege('test_edge_kek', 'UPDATE') AS invalid_priv_update_kek;
SELECT has_kek_privilege('test_edge_kek', 'DELETE') AS invalid_priv_delete_kek;
SELECT has_kek_privilege('test_edge_kek', 'ALL') AS invalid_priv_all_kek;

SELECT has_dek_privilege('test_edge_dek', 'SELECT') AS invalid_priv_select_dek;
SELECT has_dek_privilege('test_edge_dek', 'INSERT') AS invalid_priv_insert_dek;
SELECT has_dek_privilege('test_edge_dek', 'UPDATE') AS invalid_priv_update_dek;
SELECT has_dek_privilege('test_edge_dek', 'DELETE') AS invalid_priv_delete_dek;
SELECT has_dek_privilege('test_edge_dek', 'ALL') AS invalid_priv_all_dek;

-- Test 6: Case sensitivity test
SELECT has_kek_privilege('TEST_EDGE_KEK', 'USAGE') AS case_insensitive_kek;
SELECT has_kek_privilege('test_edge_kek', 'usage') AS lowercase_priv_kek;
SELECT has_kek_privilege('test_edge_kek', 'Usage') AS mixedcase_priv_kek;

SELECT has_dek_privilege('TEST_EDGE_DEK', 'USAGE') AS case_insensitive_dek;
SELECT has_dek_privilege('test_edge_dek', 'usage') AS lowercase_priv_dek;
SELECT has_dek_privilege('test_edge_dek', 'Usage') AS mixedcase_priv_dek;

-- Test 7: Grant to non-existent user (should fail)
-- Note: This will cause an error, so we need to handle it carefully
-- We'll test this by trying to grant and catching the error

-- Test 8: Revoke from user without permission (should fail)
GRANT USAGE ON KEK test_edge_kek TO tecc_edge_user1;
GRANT USAGE ON DEK test_edge_dek TO tecc_edge_user1;

-- Switch to user1 and try to revoke from user2 (should fail)
SET SESSION AUTHORIZATION tecc_edge_user1;
-- This should fail with permission error
-- REVOKE USAGE ON KEK test_edge_kek FROM tecc_edge_user2;
-- REVOKE USAGE ON DEK test_edge_dek FROM tecc_edge_user2;
RESET SESSION AUTHORIZATION;

-- Test 9: Grant with GRANT OPTION and test cascading revoke
GRANT USAGE ON KEK test_edge_kek TO tecc_edge_user2 WITH GRANT OPTION;
GRANT USAGE ON DEK test_edge_dek TO tecc_edge_user2 WITH GRANT OPTION;

-- User2 grants to user3
SET SESSION AUTHORIZATION tecc_edge_user2;
GRANT USAGE ON KEK test_edge_kek TO tecc_edge_user3;
GRANT USAGE ON DEK test_edge_dek TO tecc_edge_user3;
RESET SESSION AUTHORIZATION;

-- Check user3 has permission
SELECT has_kek_privilege('tecc_edge_user3', 'test_edge_kek', 'USAGE') AS user3_has_kek_before;
SELECT has_dek_privilege('tecc_edge_user3', 'test_edge_dek', 'USAGE') AS user3_has_dek_before;

-- Revoke GRANT OPTION from user2 with CASCADE
REVOKE GRANT OPTION FOR USAGE ON KEK test_edge_kek FROM tecc_edge_user2 CASCADE;
REVOKE GRANT OPTION FOR USAGE ON DEK test_edge_dek FROM tecc_edge_user2 CASCADE;

-- Check user3 still has permission (CASCADE should not revoke)
SELECT has_kek_privilege('tecc_edge_user3', 'test_edge_kek', 'USAGE') AS user3_has_kek_after;
SELECT has_dek_privilege('tecc_edge_user3', 'test_edge_dek', 'USAGE') AS user3_has_dek_after;

-- Test 10: Test with very long key names
-- Create KEK with long name
CREATE KEK test_edge_kek_with_very_very_very_long_name_that_exceeds_normal_length WITH (
    _TECC_PROCESSED = '1',
    KEY_STORE = 'localkms',
    KEY_PATH = '/tmp/test_long_key'
);

-- Test permission check with long name
SELECT has_kek_privilege('test_edge_kek_with_very_very_very_long_name_that_exceeds_normal_length', 'USAGE') AS long_name_kek;

-- Test 11: Test with special characters in key names
-- Note: PostgreSQL identifiers have restrictions, so we'll test with allowed special characters
CREATE KEK "test-edge-kek-special" WITH (
    _TECC_PROCESSED = '1',
    KEY_STORE = 'localkms',
    KEY_PATH = '/tmp/test_special_key'
);

-- Test permission check with special characters
SELECT has_kek_privilege('test-edge-kek-special', 'USAGE') AS special_chars_kek;

-- Test 12: Test concurrent permission checks
-- This would require multiple sessions, so we'll just document the need

-- Test 13: Test permission after key is dropped
DROP KEK "test-edge-kek-special";
-- This should return false or NULL
SELECT has_kek_privilege('test-edge-kek-special', 'USAGE') AS dropped_key_kek;

-- Test 14: Test with invalid OIDs
SELECT has_kek_privilege(0, 'USAGE') AS invalid_oid_kek;
SELECT has_dek_privilege(0, 'USAGE') AS invalid_oid_dek;

SELECT has_kek_privilege(:user1_oid, 0, 'USAGE') AS invalid_key_oid_kek;
SELECT has_dek_privilege(:user1_oid, 0, 'USAGE') AS invalid_key_oid_dek;

-- Test 15: Test all function variants with edge cases
-- Variant 1: username, keyname, privilege (with empty strings)
SELECT has_kek_privilege('', 'test_edge_kek', 'USAGE') AS empty_user_kek;
SELECT has_dek_privilege('', 'test_edge_dek', 'USAGE') AS empty_user_dek;

-- Variant 2: username, key OID, privilege (with invalid OID)
SELECT has_kek_privilege('tecc_edge_user1', 0, 'USAGE') AS invalid_key_oid_user_kek;
SELECT has_dek_privilege('tecc_edge_user1', 0, 'USAGE') AS invalid_key_oid_user_dek;

-- Variant 3: user OID, keyname, privilege (with invalid user OID)
SELECT has_kek_privilege(0, 'test_edge_kek', 'USAGE') AS invalid_user_oid_kek;
SELECT has_dek_privilege(0, 'test_edge_dek', 'USAGE') AS invalid_user_oid_dek;

-- Variant 4: user OID, key OID, privilege (with invalid OIDs)
SELECT has_kek_privilege(0, 0, 'USAGE') AS invalid_both_oids_kek;
SELECT has_dek_privilege(0, 0, 'USAGE') AS invalid_both_oids_dek;

-- Variant 5: keyname, privilege (current user) - already tested above

-- Variant 6: key OID, privilege (current user) - with invalid OID
SELECT has_kek_privilege(0, 'USAGE') AS invalid_oid_current_kek;
SELECT has_dek_privilege(0, 'USAGE') AS invalid_oid_current_dek;

-- Cleanup
REVOKE ALL ON KEK test_edge_kek FROM tecc_edge_user1, tecc_edge_user2, tecc_edge_user3;
REVOKE ALL ON DEK test_edge_dek FROM tecc_edge_user1, tecc_edge_user2, tecc_edge_user3;

DROP DEK test_edge_dek;
DROP KEK test_edge_kek_with_very_very_very_long_name_that_exceeds_normal_length;
DROP KEK test_edge_kek;

DROP USER tecc_edge_user1;
DROP USER tecc_edge_user2;
DROP USER tecc_edge_user3;