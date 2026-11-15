-- TECC administrator diagnostic functions regression test
--
-- This case requires frontend TECC processing and is run by the libpq TAP
-- regression framework.

\set ON_ERROR_STOP on

SET client_min_messages = warning;
DROP TABLE IF EXISTS tecc_admin_func_table;

DROP DEK IF EXISTS tecc_admin_func_dek;
DROP KEK IF EXISTS tecc_admin_func_kek;
DROP ROLE IF EXISTS tecc_admin_func_reader;
RESET client_min_messages;

-- Build real TECC key objects. CREATE DEK and encrypted inserts require
-- frontend TECC processing.
CREATE KEK tecc_admin_func_kek WITH (
    KEY_STORE = 'localkms',
    KEY_PATH = '%KEKPATH%/tecc_admin_func_kek.key'
);

CREATE DEK tecc_admin_func_dek WITH VALUES (
    KEK = tecc_admin_func_kek
);

CREATE TABLE tecc_admin_func_table (
    id int4,
    c_int int4_enc ENCRYPTED WITH (DEK = tecc_admin_func_dek, SUPPORT_ALGO = 'ope'),
    c_txt text_enc ENCRYPTED WITH (DEK = tecc_admin_func_dek, SUPPORT_ALGO = 'det,search'),
    note text
);

-- v2 CipherDataType layout:
-- total_len=60, version=0x00020000, dek_dist_oid=12345,
-- rnd/det/ope/search/he_add/he_mul length=4, followed by each 4-byte segment.

INSERT INTO tecc_admin_func_table (id, c_int, c_txt, note) VALUES
    (1, 10, 'alice', 'row one'),
    (2, 20, 'bob', 'row two');

-- 1. Cipher structure inspection, including all algorithm segments.
SELECT version = 131072 AS version_ok,
       dek_dist_oid = 12345::oid AS dek_oid_ok,
       rnd_len = 4 AND rnd_hex = '01020304' AS rnd_ok,
       det_len = 4 AND det_hex = '05060708' AS det_ok,
       ope_len = 4 AND ope_hex = '090a0b0c' AS ope_ok,
       search_len = 4 AND search_hex = '0d0e0f10' AS search_ok,
       he_add_len = 4 AND he_add_hex = '11121314' AS he_add_ok,
       he_mul_len = 4 AND he_mul_hex = '15161718' AS he_mul_ok,
       total_len = 60 AS total_len_ok
FROM (SELECT '\x3c00000000000200393000000400000004000000040000000400000004000000040000000102030405060708090a0b0c0d0e0f101112131415161718'::int4_enc AS c_int) s,
     tecc_enc_info(c_int);

-- 2. tecc_decrypt_raw is a frontend pseudo-function.  It follows the
-- current session decrypt mode for real encrypted columns: on returns
-- plaintext bytes hex.  OPE diagnostic decryption recovers the OPE
-- segment's normalized input independently (not via the RND payload).
\enable hg_tecc.mode
SELECT tecc_decrypt_raw(c_int, 'rnd') AS raw_rnd,
       tecc_decrypt_raw(c_int, 'ope') AS raw_ope,
       tecc_decrypt_raw(c_txt, 'det') AS raw_det
FROM tecc_admin_func_table
WHERE id = 1
\gset
SELECT length(:'raw_rnd') > 0 AS raw_mode_int_plain_hex_ok,
       length(:'raw_ope') > 0 AND :'raw_ope' <> :'raw_rnd' AS raw_mode_ope_plain_hex_ok,
       length(:'raw_det') > 0 AS raw_mode_txt_plain_hex_ok;

\disable hg_tecc.mode
\set ON_ERROR_STOP off
SELECT tecc_decrypt_raw(c_int, 'he_add') AS raw_mode_off_rejected
FROM tecc_admin_func_table
WHERE id = 1;
\set ON_ERROR_STOP on
\enable hg_tecc.mode

-- 3. Storage expansion estimator.
SELECT column_name, source_len, plain_size, enc_size, ratio::numeric(12,2) AS ratio
FROM tecc_enc_ratio('tecc_admin_func_table')
WHERE column_name <> 'TOTAL'
ORDER BY column_name;

SELECT column_name, total_plain_1m, total_enc_1m,
       overall_ratio::numeric(12,2) AS overall_ratio
FROM tecc_enc_ratio('tecc_admin_func_table')
WHERE column_name = 'TOTAL';

SELECT count(*) = 3 AS ratio_rows_ok
FROM tecc_enc_ratio('tecc_admin_func_table');

SELECT bool_and(enc_size > plain_size) AS ratio_encrypted_columns_expand_ok
FROM tecc_enc_ratio('tecc_admin_func_table')
WHERE column_name <> 'TOTAL';

SELECT total_plain_1m IS NOT NULL
       AND total_enc_1m IS NOT NULL
       AND overall_ratio IS NOT NULL AS ratio_total_ok
FROM tecc_enc_ratio('tecc_admin_func_table')
WHERE column_name = 'TOTAL';

-- 4. tecc_enc_ratio requires table-level SELECT privilege because it exposes
-- every encrypted column's metadata.  A grant on the table enables it.
CREATE ROLE tecc_admin_func_reader LOGIN;
SET ROLE tecc_admin_func_reader;
\set ON_ERROR_STOP off
SELECT count(*) FROM tecc_enc_ratio('tecc_admin_func_table');
\set ON_ERROR_STOP on
RESET ROLE;

GRANT SELECT ON tecc_admin_func_table TO tecc_admin_func_reader;
SET ROLE tecc_admin_func_reader;
SELECT count(*) = 3 AS ratio_table_select_ok
FROM tecc_enc_ratio('tecc_admin_func_table');
RESET ROLE;

-- 5. tecc_decrypt_raw additionally requires USAGE on the DEK selected by
-- the requested algorithm.  Grant KEK USAGE first so this case isolates the
-- DEK permission boundary rather than failing later while loading the KEK.
GRANT USAGE ON KEK tecc_admin_func_kek TO tecc_admin_func_reader;
\c postgres tecc_admin_func_reader
\enable hg_tecc.mode
\set ON_ERROR_STOP off
SELECT tecc_decrypt_raw(c_int, 'rnd')
FROM tecc_admin_func_table
WHERE id = 1;
\set ON_ERROR_STOP on

\c postgres postgres
GRANT USAGE ON DEK tecc_admin_func_dek TO tecc_admin_func_reader;
\c postgres tecc_admin_func_reader
\enable hg_tecc.mode
SELECT tecc_decrypt_raw(c_int, 'rnd') AS raw_with_dek_usage
FROM tecc_admin_func_table
WHERE id = 1
\gset
SELECT length(:'raw_with_dek_usage') > 0 AS raw_dek_usage_ok;

\c postgres postgres

-- 6. Ordinary columns are rejected instead of being returned unchanged.
\set ON_ERROR_STOP off
SELECT tecc_decrypt_raw(note, 'det')
FROM tecc_admin_func_table
WHERE id = 1;
\set ON_ERROR_STOP on

-- 7. An empty algorithm name must not silently fall back to RND.
\set ON_ERROR_STOP off
SELECT tecc_decrypt_raw(c_int, '')
FROM tecc_admin_func_table
WHERE id = 1;
\set ON_ERROR_STOP on

REVOKE SELECT ON tecc_admin_func_table FROM tecc_admin_func_reader;
REVOKE USAGE ON DEK tecc_admin_func_dek FROM tecc_admin_func_reader;
REVOKE USAGE ON KEK tecc_admin_func_kek FROM tecc_admin_func_reader;
DROP ROLE tecc_admin_func_reader;

DROP TABLE tecc_admin_func_table;
DROP DEK tecc_admin_func_dek;
DROP KEK tecc_admin_func_kek;

SELECT 'TECC administrator diagnostic function tests completed' AS test_status;
